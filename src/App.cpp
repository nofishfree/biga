#include "App.h"

#include "AppPaths.h"
#include "ConfigManager.h"
#include "Constants.h"
#include "Log.h"
#include "MainOverlayWindow.h"
#include "RawInputManager.h"
#include "SettingsWindow.h"
#include "StatusOverlayWindow.h"
#include "TrayController.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QMessageBox>
#include <QMetaType>
#include <QSaveFile>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>

App* App::s_instance = nullptr;

namespace {

constexpr double kMinStatusOverlayScale = 0.5;
constexpr double kMaxStatusOverlayScale = 3.0;

double normalizedStatusOverlayScale(double scale)
{
    const double clamped =
        std::clamp(scale, kMinStatusOverlayScale, kMaxStatusOverlayScale);
    return std::round(clamped * 10.0) / 10.0;
}

bool hasSavedStatusOverlayPosition(const UserConfig& config)
{
    return config.StatusOverlayX != std::numeric_limits<int>::min()
           && config.StatusOverlayY != std::numeric_limits<int>::min();
}

}  // namespace

App::App(QObject* parent)
    : QObject(parent)
{
    s_instance = this;
}

App::~App()
{
    if (m_settings != nullptr) {
        if (m_settings->isVisible()) {
            m_settings->close();
        }
        delete m_settings;
        m_settings = nullptr;
    }

    if (m_statusOverlay != nullptr) {
        m_statusOverlay->close();
    }

    if (m_capacityDebounceTimer != nullptr
        && m_capacityDebounceTimer->isActive()) {
        m_capacityDebounceTimer->stop();
        syncRuntimeCapacitiesToConfig();
        if (m_configManager) {
            m_configManager->SaveUserConfigAsync();
        }
    }
    if (m_configManager) {
        m_configManager->waitForPendingSaves();
    }

    s_instance = nullptr;
}

App* App::instance()
{
    return s_instance;
}

bool App::initialize()
{
    if (m_initialized) {
        return true;
    }

    m_configManager = std::make_unique<ConfigManager>(this);
    m_configManager->loadOnce();
    applyConfigToRuntimeCapacities();

    m_overlay = std::make_unique<MainOverlayWindow>();

    QImage storedImage;
    if (loadStoredImage(&storedImage)) {
        m_overlay->showImage(storedImage);
        Log::info(QStringLiteral("启动时显示固定图片文件"));
    } else {
        m_overlay->showDot(m_configManager->config().DotSize);
    }

    m_scopeDelayTimer = new QTimer(this);
    m_scopeDelayTimer->setSingleShot(true);
    connect(m_scopeDelayTimer, &QTimer::timeout, this,
            &App::onScopeDelayElapsed);

    m_capacityDebounceTimer = new QTimer(this);
    m_capacityDebounceTimer->setSingleShot(true);
    m_capacityDebounceTimer->setInterval(Constants::kCapacityDebounceMs);
    connect(m_capacityDebounceTimer, &QTimer::timeout, this,
            &App::onCapacityDebounceElapsed);

    m_tray = new TrayController(this);
    connect(m_tray, &TrayController::chooseImageRequested, this,
            &App::chooseImage);
    connect(m_tray, &TrayController::restoreDotRequested, this,
            &App::restoreDot);
    connect(m_tray, &TrayController::statusOverlayToggled, this,
            &App::onStatusOverlayToggled);
    connect(m_tray, &TrayController::settingsRequested, this,
            &App::openSettings);
    connect(m_tray, &TrayController::quitRequested, this, &App::requestQuit);
    m_tray->setStatusOverlayChecked(
        m_configManager->config().StatusOverlayEnabled);
    // Show after the event loop has started; this is more reliable for the
    // Windows notification-area registration than shell calls in initialize().
    QTimer::singleShot(0, m_tray, &TrayController::show);

    qRegisterMetaType<KeyBinding>("KeyBinding");
    m_rawInput = new RawInputManager(this);
    connect(m_rawInput, &RawInputManager::keyInput, this, &App::onKeyInput,
            Qt::QueuedConnection);
    connect(m_rawInput, &RawInputManager::escapePressed, this,
            &App::onEscapePressed, Qt::QueuedConnection);
    connect(m_rawInput, &RawInputManager::rightButtonPressed, this,
            &App::onRightButtonPressed, Qt::QueuedConnection);
    connect(m_rawInput, &RawInputManager::rightButtonReleased, this,
            &App::onRightButtonReleased, Qt::QueuedConnection);

    if (!m_rawInput->start()) {
        Log::error(QStringLiteral("RawInput 初始化失败，全局热键不可用"));
    }

    // StatusOverlayWindow is a QWidget, so it is created on the GUI thread
    // after the event loop starts. Exceptions are caught and do not affect the
    // overlay, tray, settings, RawInput, or persistence.
    QTimer::singleShot(0, this, [this] {
        if (createStatusOverlayIfNeeded()
            && m_configManager->config().StatusOverlayEnabled) {
            showStatusOverlayIfEnabled();
        }
    });

    m_initialized = true;
    Log::info(QStringLiteral("应用已启动"));
    return true;
}

void App::requestQuit()
{
    if (m_quitting) {
        return;
    }
    m_quitting = true;

    Log::info(QStringLiteral("应用正在退出"));

    if (m_settings != nullptr && m_settings->isVisible()) {
        m_settings->close();
    }

    if (m_statusOverlay != nullptr) {
        m_statusOverlay->close();
    }

    if (m_capacityDebounceTimer != nullptr
        && m_capacityDebounceTimer->isActive()) {
        m_capacityDebounceTimer->stop();
        syncRuntimeCapacitiesToConfig();
        if (m_configManager) {
            m_configManager->SaveUserConfigAsync();
        }
    }

    if (m_configManager) {
        m_configManager->waitForPendingSaves();
    }

    if (m_rawInput != nullptr) {
        m_rawInput->stop();
    }

    QCoreApplication::quit();
}

void App::chooseImage()
{
    const QString path = QFileDialog::getOpenFileName(
        nullptr, QStringLiteral("选择图片"), QString(),
        QStringLiteral("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (path.isEmpty()) {
        return;
    }

    QImage image;
    QString errorMessage;
    if (!persistImage(path, &image, &errorMessage)) {
        QMessageBox::warning(nullptr, QStringLiteral("BigA"), errorMessage);
        return;
    }

    m_overlay->showImage(image);
}

void App::restoreDot()
{
    m_overlay->showDot(m_configManager->config().DotSize);
}

void App::openSettings()
{
    if (m_quitting) {
        return;
    }

    if (m_settings == nullptr) {
        m_settings = new SettingsWindow();
        connect(m_settings, &SettingsWindow::configCommitted, this,
                &App::onSettingsConfigCommitted);
        connect(m_settings, &SettingsWindow::statusOverlayScalePreviewed,
                this, &App::onStatusOverlayScalePreview);
    }

    if (m_settings->isVisible()) {
        m_settings->show();
        m_settings->raise();
        m_settings->activateWindow();
        return;
    }

    m_settings->loadConfig(m_configManager->config());
    m_settings->centerOnPrimaryScreen();
    m_settings->show();
    m_settings->raise();
    m_settings->activateWindow();
}

bool App::createStatusOverlayIfNeeded()
{
    if (m_statusOverlay != nullptr) {
        return true;
    }

    try {
        m_statusOverlay = std::make_unique<StatusOverlayWindow>();
        connect(m_statusOverlay.get(), &StatusOverlayWindow::dragFinished,
                this, &App::onStatusOverlayDragFinished);

        const UserConfig& config = m_configManager->config();
        m_statusOverlay->setScale(
            normalizedStatusOverlayScale(config.StatusOverlayScale));
        refreshStatusOverlayContent();

        if (hasSavedStatusOverlayPosition(config)) {
            m_statusOverlay->restorePosition(
                QPoint(config.StatusOverlayX, config.StatusOverlayY));
        } else {
            m_statusOverlay->placeAtDefaultPosition();
        }

        m_statusOverlay->hide();
        m_statusOverlayCreationFailed = false;
        return true;
    } catch (const std::exception& exception) {
        Log::error(QStringLiteral("创建状态显示窗口失败: %1")
                       .arg(QString::fromUtf8(exception.what())));
    } catch (...) {
        Log::error(QStringLiteral("创建状态显示窗口失败: 未知异常"));
    }

    m_statusOverlay.reset();
    m_statusOverlayCreationFailed = true;
    return false;
}

void App::showStatusOverlayIfEnabled()
{
    if (m_statusOverlay == nullptr
        || !m_configManager->config().StatusOverlayEnabled) {
        return;
    }

    refreshStatusOverlayContent();
    m_statusOverlay->ensureVisibleOnScreen();
    m_statusOverlay->showNoActivate();
}

void App::refreshStatusOverlayContent()
{
    if (m_statusOverlay == nullptr) {
        return;
    }

    const int weapon = std::clamp(m_runtime.CurrentWeapon, 0, 3);
    const bool isKnife = weapon == 2;
    const int ammoState = isKnife ? 0 : ammoStateForWeapon(weapon);
    const int capacity = isKnife ? 0 : runtimeCapacityForWeapon(weapon);
    m_statusOverlay->setRuntimeStatus(weapon, ammoState, capacity);
}

void App::onStatusOverlayToggled(bool enabled)
{
    UserConfig& config = m_configManager->config();
    config.StatusOverlayEnabled = enabled;
    m_configManager->SaveUserConfigAsync();

    if (!enabled) {
        if (m_statusOverlay != nullptr) {
            m_statusOverlay->hide();
        }
        return;
    }

    if (!createStatusOverlayIfNeeded()) {
        Log::error(QStringLiteral(
            "状态显示窗口创建失败；配置保持启用，可取消后再次勾选重试"));
        return;
    }
    showStatusOverlayIfEnabled();
}

void App::onStatusOverlayScalePreview(double scale)
{
    if (m_statusOverlay == nullptr) {
        return;
    }
    m_statusOverlay->setScale(scale);
}

void App::onStatusOverlayDragFinished(const QPoint& topLeft)
{
    UserConfig& config = m_configManager->config();
    if (config.StatusOverlayX == topLeft.x()
        && config.StatusOverlayY == topLeft.y()) {
        return;
    }

    config.StatusOverlayX = topLeft.x();
    config.StatusOverlayY = topLeft.y();
    m_configManager->SaveUserConfigAsync();
}

void App::onSettingsConfigCommitted(const UserConfig& config)
{
    const UserConfig& current = m_configManager->config();

    UserConfig normalized = config;
    normalized.Weapon1Capacity =
        std::clamp(normalized.Weapon1Capacity, Constants::kMinCapacity,
                   Constants::kMaxCapacity);
    normalized.Weapon2Capacity =
        std::clamp(normalized.Weapon2Capacity, Constants::kMinCapacity,
                   Constants::kMaxCapacity);
    normalized.PistolCapacity =
        std::clamp(normalized.PistolCapacity, Constants::kMinCapacity,
                   Constants::kMaxCapacity);
    normalized.DotSize = std::max(0, normalized.DotSize);
    normalized.ScopeDelayMs = std::max(0, normalized.ScopeDelayMs);
    normalized.StatusOverlayScale =
        normalizedStatusOverlayScale(normalized.StatusOverlayScale);

    // These fields are owned by the tray action and by dragging the status
    // window, not by the settings UI. Preserve their latest values even if the
    // settings window was opened before they changed.
    normalized.StatusOverlayEnabled = current.StatusOverlayEnabled;
    normalized.StatusOverlayX = current.StatusOverlayX;
    normalized.StatusOverlayY = current.StatusOverlayY;

    m_configManager->config() = normalized;
    applyConfigToRuntimeCapacities();

    m_runtime.CurrentWeapon = 0;
    m_runtime.Weapon1AmmoState = 0;
    m_runtime.Weapon2AmmoState = 0;
    m_runtime.PistolAmmoState = 0;

    m_overlay->applyDotSize(normalized.DotSize);

    if (m_statusOverlay != nullptr) {
        m_statusOverlay->setScale(normalized.StatusOverlayScale);
    }
    refreshStatusOverlayContent();

    // The settings close is the final authority over pending +1/-1 debounce
    // work. Cancel it and save the configuration that was just committed.
    if (m_capacityDebounceTimer != nullptr) {
        m_capacityDebounceTimer->stop();
    }
    m_configManager->SaveUserConfigAsync();
}

void App::onKeyInput(const KeyBinding& binding)
{
    if (m_configManager == nullptr) {
        return;
    }

    bool functionKeyPressed = handleWeaponKey(binding);
    if (!functionKeyPressed) {
        functionKeyPressed = handleToggleKey(binding);
    }
    if (!functionKeyPressed) {
        functionKeyPressed = handleResetKey(binding);
    }
    if (!functionKeyPressed) {
        functionKeyPressed = handleCapacityKey(binding);
    }

    if (functionKeyPressed) {
        refreshStatusOverlayContent();
    }

    // If the right button is still being held and the initial hide delay has
    // already elapsed, any bound function key can change the current state.
    // Re-evaluate immediately instead of waiting for another right press.
    // Before the delay expires, the pending timer will evaluate the latest
    // state when it fires, preserving ScopeDelayMs semantics.
    if (m_rightButtonHeld && functionKeyPressed
        && m_scopeDelayTimer != nullptr && !m_scopeDelayTimer->isActive()) {
        updateScopeVisibility();
    }

    if (m_settings != nullptr && m_settings->isVisible()) {
        m_settings->handleRawInput(binding);
    }
}

void App::onEscapePressed()
{
    if (m_settings != nullptr && m_settings->isVisible()) {
        m_settings->handleEscapePressed();
    }
}

void App::onRightButtonPressed()
{
    m_rightButtonHeld = true;
    m_scopeDelayTimer->stop();
    m_scopeDelayTimer->start(
        std::max(0, m_configManager->config().ScopeDelayMs));
}

void App::onRightButtonReleased()
{
    m_rightButtonHeld = false;
    m_scopeDelayTimer->stop();
    m_overlay->showCurrent();
}

void App::onScopeDelayElapsed()
{
    if (!m_rightButtonHeld) {
        return;
    }
    updateScopeVisibility();
}

bool App::shouldHideOverlayForScope() const
{
    if (m_runtime.CurrentWeapon == 2) {
        return true;
    }

    const int capacity = runtimeCapacityForWeapon(m_runtime.CurrentWeapon);
    if (capacity <= 0) {
        return false;
    }

    const int ammoState = ammoStateForWeapon(m_runtime.CurrentWeapon);
    return ammoState != capacity - 1;
}

void App::updateScopeVisibility()
{
    if (!m_rightButtonHeld) {
        return;
    }

    if (shouldHideOverlayForScope()) {
        m_overlay->hideForScope();
    } else if (!m_overlay->isVisible()) {
        m_overlay->showCurrent();
    }
}

void App::onCapacityDebounceElapsed()
{
    syncRuntimeCapacitiesToConfig();
    m_configManager->SaveUserConfigAsync();
}

bool App::handleWeaponKey(const KeyBinding& binding)
{
    const UserConfig& config = m_configManager->config();

    if (binding == config.Weapon1Key1 || binding == config.Weapon1Key2) {
        m_runtime.CurrentWeapon = 0;
        return true;
    }
    if (binding == config.Weapon2Key1 || binding == config.Weapon2Key2) {
        m_runtime.CurrentWeapon = 1;
        return true;
    }
    if (binding == config.KnifeKey1 || binding == config.KnifeKey2) {
        m_runtime.CurrentWeapon = 2;
        return true;
    }
    if (binding == config.PistolKey1 || binding == config.PistolKey2) {
        m_runtime.CurrentWeapon = 3;
        return true;
    }
    return false;
}

bool App::handleToggleKey(const KeyBinding& binding)
{
    const UserConfig& config = m_configManager->config();
    if (binding != config.ToggleKey1 && binding != config.ToggleKey2) {
        return false;
    }

    toggleCurrentWeaponAmmo();
    return true;
}

bool App::handleResetKey(const KeyBinding& binding)
{
    const UserConfig& config = m_configManager->config();
    if (binding != config.ResetKey) {
        return false;
    }

    resetRuntime();
    return true;
}

bool App::handleCapacityKey(const KeyBinding& binding)
{
    const UserConfig& config = m_configManager->config();

    if (binding == config.CapacityIncKey) {
        adjustCurrentCapacity(1);
        return true;
    }
    if (binding == config.CapacityDecKey) {
        adjustCurrentCapacity(-1);
        return true;
    }
    return false;
}

void App::toggleCurrentWeaponAmmo()
{
    if (m_runtime.CurrentWeapon == 2) {
        return;
    }

    const int capacity = runtimeCapacityForWeapon(m_runtime.CurrentWeapon);
    if (capacity == 0) {
        return;
    }

    int& state = ammoStateForWeapon(m_runtime.CurrentWeapon);
    state = (state + 1) % capacity;
}

void App::resetRuntime()
{
    m_runtime.CurrentWeapon = 0;
    m_runtime.Weapon1AmmoState = 0;
    m_runtime.Weapon2AmmoState = 0;
    m_runtime.PistolAmmoState = 0;
}

int& App::runtimeCapacityForWeapon(int weapon)
{
    switch (weapon) {
    case 0:
        return m_runtime.RuntimeWeapon1Capacity;
    case 1:
        return m_runtime.RuntimeWeapon2Capacity;
    case 3:
        return m_runtime.RuntimePistolCapacity;
    default:
        return m_runtime.RuntimeWeapon1Capacity;
    }
}

const int& App::runtimeCapacityForWeapon(int weapon) const
{
    switch (weapon) {
    case 0:
        return m_runtime.RuntimeWeapon1Capacity;
    case 1:
        return m_runtime.RuntimeWeapon2Capacity;
    case 3:
        return m_runtime.RuntimePistolCapacity;
    default:
        return m_runtime.RuntimeWeapon1Capacity;
    }
}

int& App::ammoStateForWeapon(int weapon)
{
    switch (weapon) {
    case 0:
        return m_runtime.Weapon1AmmoState;
    case 1:
        return m_runtime.Weapon2AmmoState;
    default:
        return m_runtime.PistolAmmoState;
    }
}

const int& App::ammoStateForWeapon(int weapon) const
{
    switch (weapon) {
    case 0:
        return m_runtime.Weapon1AmmoState;
    case 1:
        return m_runtime.Weapon2AmmoState;
    default:
        return m_runtime.PistolAmmoState;
    }
}

void App::adjustCurrentCapacity(int delta)
{
    if (m_runtime.CurrentWeapon == 2) {
        // Knife: no capacity change, no timer reset and no save.
        return;
    }

    int& capacity = runtimeCapacityForWeapon(m_runtime.CurrentWeapon);
    capacity = std::clamp(capacity + delta, Constants::kMinCapacity,
                          Constants::kMaxCapacity);

    // Boundary presses still restart the debounce and eventually save.
    m_capacityDebounceTimer->start();
}

void App::syncRuntimeCapacitiesToConfig()
{
    UserConfig& config = m_configManager->config();
    config.Weapon1Capacity = m_runtime.RuntimeWeapon1Capacity;
    config.Weapon2Capacity = m_runtime.RuntimeWeapon2Capacity;
    config.PistolCapacity = m_runtime.RuntimePistolCapacity;
}

void App::applyConfigToRuntimeCapacities()
{
    UserConfig& config = m_configManager->config();
    config.Weapon1Capacity =
        std::clamp(config.Weapon1Capacity, Constants::kMinCapacity,
                   Constants::kMaxCapacity);
    config.Weapon2Capacity =
        std::clamp(config.Weapon2Capacity, Constants::kMinCapacity,
                   Constants::kMaxCapacity);
    config.PistolCapacity =
        std::clamp(config.PistolCapacity, Constants::kMinCapacity,
                   Constants::kMaxCapacity);
    config.DotSize = std::max(0, config.DotSize);
    config.ScopeDelayMs = std::max(0, config.ScopeDelayMs);

    m_runtime.RuntimeWeapon1Capacity = config.Weapon1Capacity;
    m_runtime.RuntimeWeapon2Capacity = config.Weapon2Capacity;
    m_runtime.RuntimePistolCapacity = config.PistolCapacity;
}

bool App::loadStoredImage(QImage* image)
{
    if (image == nullptr) {
        return false;
    }

    const QString path = AppPaths::imageFilePath();
    if (!QFileInfo::exists(path)) {
        return false;
    }

    QImageReader reader(path);
    reader.setDecideFormatFromContent(true);
    reader.setAutoTransform(false);

    if (!reader.canRead()) {
        Log::warning(QStringLiteral("固定图片文件存在但不可读，回退白点: %1 (%2)")
                         .arg(path, reader.errorString()));
        return false;
    }

    const QImage loaded = reader.read();
    if (loaded.isNull()) {
        Log::warning(QStringLiteral("固定图片文件解码失败，回退白点: %1 (%2)")
                         .arg(path, reader.errorString()));
        return false;
    }

    *image = loaded;
    return true;
}

bool App::persistImage(const QString& sourcePath, QImage* image,
                       QString* errorMessage)
{
    auto fail = [errorMessage](const QString& message) {
        if (errorMessage != nullptr) {
            *errorMessage = message;
        }
        Log::warning(message);
        return false;
    };

    if (image == nullptr) {
        return fail(QStringLiteral("内部错误：图片输出为空"));
    }

    QImageReader reader(sourcePath);
    reader.setDecideFormatFromContent(true);
    reader.setAutoTransform(false);
    if (!reader.canRead()) {
        return fail(QStringLiteral("无法读取所选图片：%1")
                        .arg(reader.errorString()));
    }

    const QImage loaded = reader.read();
    if (loaded.isNull()) {
        return fail(QStringLiteral("所选图片解码失败：%1")
                        .arg(reader.errorString()));
    }

    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        return fail(QStringLiteral("无法打开所选图片：%1")
                        .arg(source.errorString()));
    }
    const QByteArray bytes = source.readAll();
    source.close();
    if (bytes.isEmpty()) {
        return fail(QStringLiteral("所选图片文件为空"));
    }

    if (!AppPaths::ensureDataDir()) {
        return fail(QStringLiteral("无法创建应用数据目录：%1")
                        .arg(AppPaths::dataDir()));
    }

    QSaveFile output(AppPaths::imageFilePath());
    if (!output.open(QIODevice::WriteOnly)) {
        return fail(QStringLiteral("无法写入固定图片文件：%1")
                        .arg(output.errorString()));
    }
    if (output.write(bytes) != bytes.size()) {
        const QString error = output.errorString();
        output.cancelWriting();
        return fail(QStringLiteral("写入固定图片文件失败：%1").arg(error));
    }
    if (!output.commit()) {
        return fail(QStringLiteral("提交固定图片文件失败：%1")
                        .arg(output.errorString()));
    }

    *image = loaded;
    return true;
}
