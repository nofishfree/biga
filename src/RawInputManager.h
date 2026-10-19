#pragma once

#include "KeyBinding.h"

#include <QObject>
#include <QSet>

#include <memory>

class RawInputManager final : public QObject {
    Q_OBJECT

public:
    explicit RawInputManager(QObject* parent = nullptr);
    ~RawInputManager() override;

    bool start();
    void stop();

signals:
    void keyInput(const KeyBinding& binding);
    void escapePressed();
    void rightButtonPressed();
    void rightButtonReleased();

private:
    struct NativeWindow;

    void processRawInputMessage(quintptr wParam, qintptr lParam);
    void processKeyboard(const void* rawKeyboard);
    void processMouse(const void* rawMouse);
    bool registerDevices();

    std::unique_ptr<NativeWindow> m_nativeWindow;
    bool m_started = false;
    bool m_devicesRegistered = false;

    QSet<quint32> m_pressedKeys;

    bool m_rightDown = false;
    bool m_middleDown = false;
    bool m_x1Down = false;
    bool m_x2Down = false;
};
