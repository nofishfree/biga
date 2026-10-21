#pragma once

#include "KeyBinding.h"
#include "RuntimeState.h"
#include "UserConfig.h"

#include <QObject>
#include <QPoint>

#include <memory>

class ConfigManager;
class MainOverlayWindow;
class QImage;
class RawInputManager;
class SettingsWindow;
class StatusOverlayWindow;
class TrayController;
class QTimer;

class App final : public QObject {
    Q_OBJECT

public:
    explicit App(QObject* parent = nullptr);
    ~App() override;

    static App* instance();

    bool initialize();
    void requestQuit();

private:
    void chooseImage();
    void restoreDot();
    void openSettings();

    void onSettingsConfigCommitted(const UserConfig& config);

    bool createStatusOverlayIfNeeded();
    void showStatusOverlayIfEnabled();
    void refreshStatusOverlayContent();
    void onStatusOverlayToggled(bool enabled);
    void onStatusOverlayScalePreview(double scale);
    void onStatusOverlayDragFinished(const QPoint& topLeft);

    void onKeyInput(const KeyBinding& binding);
    void onEscapePressed();
    void onRightButtonPressed();
    void onRightButtonReleased();
    void onScopeDelayElapsed();
    void onCapacityDebounceElapsed();

    bool handleWeaponKey(const KeyBinding& binding);
    bool handleToggleKey(const KeyBinding& binding);
    bool handleResetKey(const KeyBinding& binding);
    bool handleCapacityKey(const KeyBinding& binding);

    void toggleCurrentWeaponAmmo();
    void resetRuntime();

    int& runtimeCapacityForWeapon(int weapon);
    const int& runtimeCapacityForWeapon(int weapon) const;
    int& ammoStateForWeapon(int weapon);
    const int& ammoStateForWeapon(int weapon) const;

    void adjustCurrentCapacity(int delta);
    void syncRuntimeCapacitiesToConfig();
    void applyConfigToRuntimeCapacities();
    bool shouldHideOverlayForScope() const;
    void updateScopeVisibility();

    static bool loadStoredImage(QImage* image);
    static bool persistImage(const QString& sourcePath, QImage* image,
                             QString* errorMessage);

    static App* s_instance;

    std::unique_ptr<ConfigManager> m_configManager;
    std::unique_ptr<MainOverlayWindow> m_overlay;
    std::unique_ptr<StatusOverlayWindow> m_statusOverlay;
    RawInputManager* m_rawInput = nullptr;
    TrayController* m_tray = nullptr;
    SettingsWindow* m_settings = nullptr;

    QTimer* m_scopeDelayTimer = nullptr;
    QTimer* m_capacityDebounceTimer = nullptr;

    RuntimeState m_runtime;
    bool m_rightButtonHeld = false;
    bool m_statusOverlayCreationFailed = false;
    bool m_quitting = false;
    bool m_initialized = false;
};
