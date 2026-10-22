#pragma once

#include "KeyBinding.h"
#include "UserConfig.h"

#include <QVector>
#include <QWidget>

class CapacityEdit;
class FramelessTitleBar;
class KeyBindingEdit;
class ParameterEdit;
class QCloseEvent;
class QGridLayout;
class QKeyEvent;
class QLabel;
class QShowEvent;
class QSlider;

class SettingsWindow final : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget* parent = nullptr);

    void loadConfig(const UserConfig& config);
    void centerOnPrimaryScreen();

signals:
    void configCommitted(const UserConfig& config);
    void statusOverlayScalePreviewed(double scale);

public slots:
    void handleRawInput(const KeyBinding& binding);
    void handleEscapePressed();

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    struct BindingSlot {
        KeyBindingEdit* editor = nullptr;
        KeyBinding UserConfig::*member = nullptr;
        QString duplicateLabel;
    };

    void buildUi();
    QWidget* createSection(const QString& title, QGridLayout** gridOut);
    QLabel* makeFieldLabel(const QString& text);
    QLabel* makeCapacityLabel(const QString& text);
    KeyBindingEdit* makeBindingEditor(QGridLayout* grid, int row, int column,
                                      KeyBinding UserConfig::*member,
                                      const QString& duplicateLabel);
    void addBindingSlot(KeyBindingEdit* editor,
                        KeyBinding UserConfig::*member,
                        const QString& duplicateLabel);

    BindingSlot* slotFor(KeyBindingEdit* editor);

    void onCaptureStarted(KeyBindingEdit* editor);
    void onCaptureCancelled(KeyBindingEdit* editor);
    void onBindingChanged(KeyBindingEdit* editor);
    void onBindingCleared(KeyBindingEdit* editor);

    void loadBindings();
    UserConfig collectConfig() const;
    void onStatusScaleSliderChanged(int value);
    void updateStatusScaleValueLabel();

    FramelessTitleBar* m_titleBar = nullptr;

    KeyBindingEdit* m_toggleKey1 = nullptr;
    KeyBindingEdit* m_toggleKey2 = nullptr;
    KeyBindingEdit* m_resetKey = nullptr;

    KeyBindingEdit* m_weapon1Key1 = nullptr;
    KeyBindingEdit* m_weapon1Key2 = nullptr;
    KeyBindingEdit* m_weapon2Key1 = nullptr;
    KeyBindingEdit* m_weapon2Key2 = nullptr;
    KeyBindingEdit* m_knifeKey1 = nullptr;
    KeyBindingEdit* m_knifeKey2 = nullptr;
    KeyBindingEdit* m_pistolKey1 = nullptr;
    KeyBindingEdit* m_pistolKey2 = nullptr;

    KeyBindingEdit* m_capacityIncKey = nullptr;
    KeyBindingEdit* m_capacityDecKey = nullptr;

    CapacityEdit* m_weapon1Capacity = nullptr;
    CapacityEdit* m_weapon2Capacity = nullptr;
    CapacityEdit* m_pistolCapacity = nullptr;

    ParameterEdit* m_dotSize = nullptr;
    ParameterEdit* m_scopeDelay = nullptr;
    QSlider* m_statusScaleSlider = nullptr;
    QLabel* m_statusScaleValueLabel = nullptr;

    QVector<BindingSlot> m_bindingSlots;
    KeyBindingEdit* m_activeCapture = nullptr;
    bool m_confirmOpen = false;
    bool m_closing = false;

    UserConfig m_workingConfig;
};
