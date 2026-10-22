#include "SettingsWindow.h"

#include "widgets/CapacityEdit.h"
#include "widgets/FramelessTitleBar.h"
#include "widgets/KeyBindingEdit.h"
#include "widgets/ParameterEdit.h"

#include <QCloseEvent>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSlider>
#include <QStyle>
#include <QVBoxLayout>

#include <cmath>

#include <windows.h>

namespace {

constexpr int kFieldLabelWidth = 128;
constexpr int kBindingWidth = 156;
constexpr int kCapacityLabelWidth = 76;
constexpr int kNumberWidth = 108;
constexpr int kControlHeight = 32;
constexpr int kWindowWidth = 760;
constexpr int kScaleLabelWidth = 44;

}  // namespace

SettingsWindow::SettingsWindow(QWidget* parent)
    : QWidget(parent)
    , m_workingConfig(UserConfig::defaults())
{
    setObjectName(QStringLiteral("settingsWindow"));
    setWindowTitle(QStringLiteral("设置"));
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint);
    setWindowModality(Qt::NonModal);
    setAttribute(Qt::WA_StyledBackground, true);

    buildUi();

    QFile styleFile(QStringLiteral(":/styles/settings.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    setFixedWidth(kWindowWidth);
    if (layout() != nullptr) {
        layout()->activate();
    }
    adjustSize();
    setFixedHeight(sizeHint().height());

    loadConfig(UserConfig::defaults());
}

void SettingsWindow::loadConfig(const UserConfig& config)
{
    if (m_activeCapture != nullptr) {
        KeyBindingEdit* active = m_activeCapture;
        m_activeCapture = nullptr;
        active->cancelCapture();
    }
    m_confirmOpen = false;
    m_workingConfig = config;
    loadBindings();

    m_weapon1Capacity->setValue(config.Weapon1Capacity);
    m_weapon2Capacity->setValue(config.Weapon2Capacity);
    m_pistolCapacity->setValue(config.PistolCapacity);
    m_dotSize->setValue(config.DotSize);
    m_scopeDelay->setValue(config.ScopeDelayMs);

    const QSignalBlocker blocker(m_statusScaleSlider);
    const int scaleTenths =
        std::clamp(static_cast<int>(std::lround(config.StatusOverlayScale * 10.0)),
                   5, 30);
    m_statusScaleSlider->setValue(scaleTenths);
    updateStatusScaleValueLabel();
}

void SettingsWindow::centerOnPrimaryScreen()
{
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen == nullptr) {
        return;
    }
    const QRect available = screen->availableGeometry();
    const int x = available.x()
                  + static_cast<int>(std::floor(
                        (available.width() - width()) / 2.0));
    const int y = available.y()
                  + static_cast<int>(std::floor(
                        (available.height() - height()) / 2.0));
    move(x, y);
}

void SettingsWindow::handleRawInput(const KeyBinding& binding)
{
    if (m_activeCapture == nullptr || m_confirmOpen || binding.isEmpty()) {
        return;
    }

    BindingSlot* destination = slotFor(m_activeCapture);
    if (destination == nullptr) {
        return;
    }

    for (const BindingSlot& slot : m_bindingSlots) {
        if (slot.editor == destination->editor || slot.editor->binding().isEmpty()
            || slot.editor->binding() != binding) {
            continue;
        }

        m_confirmOpen = true;

        QMessageBox box(this);
        box.setWindowTitle(QStringLiteral("按键冲突"));
        box.setIcon(QMessageBox::Question);
        box.setText(QStringLiteral("该键已绑定到 %1，是否解绑并重新绑定？")
                        .arg(slot.duplicateLabel));
        QPushButton* rebindButton =
            box.addButton(QStringLiteral("解绑并重新绑定"),
                          QMessageBox::AcceptRole);
        QPushButton* cancelButton =
            box.addButton(QStringLiteral("取消"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelButton);
        box.setEscapeButton(cancelButton);
        box.exec();

        const bool accepted = box.clickedButton() == rebindButton;
        m_confirmOpen = false;

        if (accepted) {
            slot.editor->clearBinding();
            if (m_activeCapture != nullptr) {
                m_activeCapture->applyBinding(binding);
            }
        }
        // Rejected: stay in capture mode and wait for another key.
        return;
    }

    m_activeCapture->applyBinding(binding);
}

void SettingsWindow::handleEscapePressed()
{
    if (m_activeCapture != nullptr) {
        m_activeCapture->cancelCapture();
    }
}

void SettingsWindow::closeEvent(QCloseEvent* event)
{
    if (m_closing) {
        event->accept();
        return;
    }
    m_closing = true;

    if (m_activeCapture != nullptr) {
        KeyBindingEdit* active = m_activeCapture;
        m_activeCapture = nullptr;
        active->cancelCapture();
    }

    // A close click can arrive before the parameter editor got focusOutEvent.
    m_dotSize->forceCommit();
    m_scopeDelay->forceCommit();

    const UserConfig committed = collectConfig();
    m_workingConfig = committed;
    emit configCommitted(committed);

    event->accept();
    QWidget::closeEvent(event);
    m_closing = false;
}

void SettingsWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape && m_activeCapture != nullptr) {
        handleEscapePressed();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void SettingsWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd != nullptr) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

void SettingsWindow::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_titleBar = new FramelessTitleBar(this);
    rootLayout->addWidget(m_titleBar);
    connect(m_titleBar, &FramelessTitleBar::closeRequested, this,
            &QWidget::close);

    auto* content = new QWidget(this);
    content->setObjectName(QStringLiteral("settingsContent"));
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(20, 16, 20, 20);
    contentLayout->setSpacing(16);
    rootLayout->addWidget(content);

    QGridLayout* group1Grid = nullptr;
    QWidget* group1 = createSection(QStringLiteral("切换与重置"), &group1Grid);
    contentLayout->addWidget(group1);

    group1Grid->addWidget(makeFieldLabel(QStringLiteral("切换")), 0, 0);
    m_toggleKey1 = makeBindingEditor(group1Grid, 0, 1, &UserConfig::ToggleKey1,
                                     QStringLiteral("切换键 1"));
    m_toggleKey2 = makeBindingEditor(group1Grid, 0, 2, &UserConfig::ToggleKey2,
                                     QStringLiteral("切换键 2"));
    group1Grid->addWidget(makeFieldLabel(QStringLiteral("重置/恢复")), 1, 0);
    m_resetKey = makeBindingEditor(group1Grid, 1, 1, &UserConfig::ResetKey,
                                   QStringLiteral("重置键"));
    group1Grid->addWidget(makeFieldLabel(QStringLiteral("白点大小")), 2, 0);
    m_dotSize = new ParameterEdit(this);
    m_dotSize->setFixedSize(kNumberWidth, kControlHeight);
    group1Grid->addWidget(m_dotSize, 2, 1);

    QGridLayout* group2Grid = nullptr;
    QWidget* group2 = createSection(QStringLiteral("武器热键"), &group2Grid);
    contentLayout->addWidget(group2);

    group2Grid->addWidget(makeFieldLabel(QStringLiteral("一号武器")), 0, 0);
    m_weapon1Key1 = makeBindingEditor(
        group2Grid, 0, 1, &UserConfig::Weapon1Key1,
        QStringLiteral("一号武器键 1"));
    m_weapon1Key2 = makeBindingEditor(
        group2Grid, 0, 2, &UserConfig::Weapon1Key2,
        QStringLiteral("一号武器键 2"));
    group2Grid->addWidget(makeCapacityLabel(QStringLiteral("一号容量")), 0, 3);
    m_weapon1Capacity = new CapacityEdit(this);
    m_weapon1Capacity->setFixedSize(kNumberWidth, kControlHeight);
    group2Grid->addWidget(m_weapon1Capacity, 0, 4);

    group2Grid->addWidget(makeFieldLabel(QStringLiteral("二号武器")), 1, 0);
    m_weapon2Key1 = makeBindingEditor(
        group2Grid, 1, 1, &UserConfig::Weapon2Key1,
        QStringLiteral("二号武器键 1"));
    m_weapon2Key2 = makeBindingEditor(
        group2Grid, 1, 2, &UserConfig::Weapon2Key2,
        QStringLiteral("二号武器键 2"));
    group2Grid->addWidget(makeCapacityLabel(QStringLiteral("二号容量")), 1, 3);
    m_weapon2Capacity = new CapacityEdit(this);
    m_weapon2Capacity->setFixedSize(kNumberWidth, kControlHeight);
    group2Grid->addWidget(m_weapon2Capacity, 1, 4);

    group2Grid->addWidget(makeFieldLabel(QStringLiteral("刀")), 2, 0);
    m_knifeKey1 = makeBindingEditor(group2Grid, 2, 1, &UserConfig::KnifeKey1,
                                    QStringLiteral("刀键 1"));
    m_knifeKey2 = makeBindingEditor(group2Grid, 2, 2, &UserConfig::KnifeKey2,
                                    QStringLiteral("刀键 2"));

    group2Grid->addWidget(makeFieldLabel(QStringLiteral("手枪")), 3, 0);
    m_pistolKey1 = makeBindingEditor(
        group2Grid, 3, 1, &UserConfig::PistolKey1,
        QStringLiteral("手枪键 1"));
    m_pistolKey2 = makeBindingEditor(
        group2Grid, 3, 2, &UserConfig::PistolKey2,
        QStringLiteral("手枪键 2"));
    group2Grid->addWidget(makeCapacityLabel(QStringLiteral("手枪容量")), 3, 3);
    m_pistolCapacity = new CapacityEdit(this);
    m_pistolCapacity->setFixedSize(kNumberWidth, kControlHeight);
    group2Grid->addWidget(m_pistolCapacity, 3, 4);

    group2Grid->addWidget(makeFieldLabel(QStringLiteral("容量+1")), 4, 0);
    m_capacityIncKey = makeBindingEditor(
        group2Grid, 4, 1, &UserConfig::CapacityIncKey,
        QStringLiteral("容量+1 键"));
    group2Grid->addWidget(makeFieldLabel(QStringLiteral("容量-1")), 5, 0);
    m_capacityDecKey = makeBindingEditor(
        group2Grid, 5, 1, &UserConfig::CapacityDecKey,
        QStringLiteral("容量-1 键"));

    QGridLayout* group3Grid = nullptr;
    QWidget* group3 = createSection(QStringLiteral("参数"), &group3Grid);
    contentLayout->addWidget(group3);

    group3Grid->addWidget(makeFieldLabel(QStringLiteral("开镜延迟(ms)")), 0, 0);
    m_scopeDelay = new ParameterEdit(this);
    m_scopeDelay->setFixedSize(kNumberWidth, kControlHeight);
    group3Grid->addWidget(m_scopeDelay, 0, 1);

    group3Grid->addWidget(
        makeFieldLabel(QStringLiteral("状态显示缩放比例")), 1, 0);
    auto* scaleControl = new QWidget(this);
    auto* scaleLayout = new QHBoxLayout(scaleControl);
    scaleLayout->setContentsMargins(0, 0, 0, 0);
    scaleLayout->setSpacing(8);
    m_statusScaleSlider = new QSlider(Qt::Horizontal, scaleControl);
    m_statusScaleSlider->setObjectName(QStringLiteral("statusScaleSlider"));
    m_statusScaleSlider->setRange(5, 30);
    m_statusScaleSlider->setSingleStep(1);
    m_statusScaleSlider->setPageStep(5);
    m_statusScaleSlider->setValue(10);
    m_statusScaleSlider->setFixedHeight(kControlHeight);
    m_statusScaleValueLabel = new QLabel(QStringLiteral("1.0"), scaleControl);
    m_statusScaleValueLabel->setObjectName(QStringLiteral("scaleValueLabel"));
    m_statusScaleValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_statusScaleValueLabel->setFixedWidth(kScaleLabelWidth);
    scaleLayout->addWidget(m_statusScaleSlider, 1);
    scaleLayout->addWidget(m_statusScaleValueLabel, 0);
    group3Grid->addWidget(scaleControl, 1, 1, 1, 4);
    connect(m_statusScaleSlider, &QSlider::valueChanged, this,
            &SettingsWindow::onStatusScaleSliderChanged);

    contentLayout->addStretch(1);
}

QWidget* SettingsWindow::createSection(const QString& title,
                                       QGridLayout** gridOut)
{
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("sectionCard"));
    card->setFrameShape(QFrame::NoFrame);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 12, 14, 14);
    cardLayout->setSpacing(10);

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("sectionTitle"));
    cardLayout->addWidget(titleLabel);

    auto* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(8);
    grid->setColumnMinimumWidth(0, kFieldLabelWidth);
    grid->setColumnMinimumWidth(1, kBindingWidth);
    grid->setColumnMinimumWidth(2, kBindingWidth);
    grid->setColumnMinimumWidth(3, kCapacityLabelWidth);
    grid->setColumnMinimumWidth(4, kNumberWidth);
    grid->setColumnStretch(5, 1);
    cardLayout->addLayout(grid);

    *gridOut = grid;
    return card;
}

QLabel* SettingsWindow::makeFieldLabel(const QString& text)
{
    auto* label = new QLabel(text, this);
    label->setObjectName(QStringLiteral("fieldLabel"));
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    label->setFixedWidth(kFieldLabelWidth);
    return label;
}

QLabel* SettingsWindow::makeCapacityLabel(const QString& text)
{
    auto* label = new QLabel(text, this);
    label->setObjectName(QStringLiteral("capacityLabel"));
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    label->setFixedWidth(kCapacityLabelWidth);
    return label;
}

KeyBindingEdit* SettingsWindow::makeBindingEditor(
    QGridLayout* grid, int row, int column, KeyBinding UserConfig::*member,
    const QString& duplicateLabel)
{
    auto* editor = new KeyBindingEdit(this);
    editor->setFixedSize(kBindingWidth, kControlHeight);
    grid->addWidget(editor, row, column);
    addBindingSlot(editor, member, duplicateLabel);
    return editor;
}

void SettingsWindow::addBindingSlot(KeyBindingEdit* editor,
                                    KeyBinding UserConfig::*member,
                                    const QString& duplicateLabel)
{
    m_bindingSlots.append(BindingSlot{editor, member, duplicateLabel});

    connect(editor, &KeyBindingEdit::captureStarted, this,
            &SettingsWindow::onCaptureStarted);
    connect(editor, &KeyBindingEdit::captureCancelled, this,
            &SettingsWindow::onCaptureCancelled);
    connect(editor, &KeyBindingEdit::bindingChanged, this,
            &SettingsWindow::onBindingChanged);
    connect(editor, &KeyBindingEdit::bindingCleared, this,
            &SettingsWindow::onBindingCleared);
}

SettingsWindow::BindingSlot* SettingsWindow::slotFor(KeyBindingEdit* editor)
{
    for (BindingSlot& slot : m_bindingSlots) {
        if (slot.editor == editor) {
            return &slot;
        }
    }
    return nullptr;
}

void SettingsWindow::onCaptureStarted(KeyBindingEdit* editor)
{
    if (m_activeCapture == editor) {
        return;
    }

    if (m_activeCapture != nullptr) {
        KeyBindingEdit* previous = m_activeCapture;
        m_activeCapture = nullptr;
        previous->cancelCapture();
    }
    m_activeCapture = editor;
}

void SettingsWindow::onCaptureCancelled(KeyBindingEdit* editor)
{
    if (m_activeCapture == editor) {
        m_activeCapture = nullptr;
    }
}

void SettingsWindow::onBindingChanged(KeyBindingEdit* editor)
{
    if (m_activeCapture == editor) {
        m_activeCapture = nullptr;
    }
}

void SettingsWindow::onBindingCleared(KeyBindingEdit* editor)
{
    if (m_activeCapture == editor) {
        m_activeCapture = nullptr;
    }
}

void SettingsWindow::loadBindings()
{
    for (BindingSlot& slot : m_bindingSlots) {
        slot.editor->setBindingSilently(m_workingConfig.*(slot.member));
    }
}

void SettingsWindow::onStatusScaleSliderChanged(int value)
{
    Q_UNUSED(value);
    updateStatusScaleValueLabel();
    emit statusOverlayScalePreviewed(m_statusScaleSlider->value() / 10.0);
}

void SettingsWindow::updateStatusScaleValueLabel()
{
    if (m_statusScaleValueLabel == nullptr || m_statusScaleSlider == nullptr) {
        return;
    }
    m_statusScaleValueLabel->setText(
        QString::number(m_statusScaleSlider->value() / 10.0, 'f', 1));
}

UserConfig SettingsWindow::collectConfig() const
{
    UserConfig config = m_workingConfig;
    for (const BindingSlot& slot : m_bindingSlots) {
        config.*(slot.member) = slot.editor->binding();
    }

    config.Weapon1Capacity = m_weapon1Capacity->value();
    config.Weapon2Capacity = m_weapon2Capacity->value();
    config.PistolCapacity = m_pistolCapacity->value();
    config.DotSize = m_dotSize->value();
    config.ScopeDelayMs = m_scopeDelay->value();
    config.StatusOverlayScale = m_statusScaleSlider->value() / 10.0;
    return config;
}
