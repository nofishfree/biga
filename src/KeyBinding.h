#pragma once

#include <QJsonValue>
#include <QMetaType>
#include <QString>
#include <QtGlobal>

enum class KeyBindingType {
    None = 0,
    Keyboard,
    MouseMiddle,
    MouseX1,
    MouseX2,
    WheelUp,
    WheelDown,
};

struct KeyBinding {
    KeyBindingType type = KeyBindingType::None;

    // Keyboard identity. virtualKey is normalized for the numpad so that
    // NumLock on/off produces the same binding.
    quint16 scanCode = 0;
    bool extended = false;
    quint16 virtualKey = 0;

    bool isEmpty() const { return type == KeyBindingType::None; }
    bool isKeyboard() const { return type == KeyBindingType::Keyboard; }

    QString displayName() const;

    QJsonValue toJsonValue() const;

    static KeyBinding fromRawKeyboard(quint16 scanCode, bool extended,
                                      quint16 virtualKey);
    static quint16 canonicalVirtualKey(quint16 scanCode, bool extended,
                                       quint16 virtualKey);

    static KeyBinding mouseMiddle();
    static KeyBinding mouseX1();
    static KeyBinding mouseX2();
    static KeyBinding wheelUp();
    static KeyBinding wheelDown();

    bool operator==(const KeyBinding& other) const;
    bool operator!=(const KeyBinding& other) const { return !(*this == other); }
};

Q_DECLARE_METATYPE(KeyBinding)
