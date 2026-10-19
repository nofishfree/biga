#include "KeyBinding.h"

#include <QChar>
#include <QJsonObject>

#include <iterator>

#include <windows.h>

namespace {

QString numpadDisplayName(quint16 scanCode, bool extended)
{
    if (!extended && scanCode >= 0x47 && scanCode <= 0x53) {
        switch (scanCode) {
        case 0x47:
            return QStringLiteral("小键盘 7");
        case 0x48:
            return QStringLiteral("小键盘 8");
        case 0x49:
            return QStringLiteral("小键盘 9");
        case 0x4A:
            return QStringLiteral("小键盘 -");
        case 0x4B:
            return QStringLiteral("小键盘 4");
        case 0x4C:
            return QStringLiteral("小键盘 5");
        case 0x4D:
            return QStringLiteral("小键盘 6");
        case 0x4E:
            return QStringLiteral("小键盘 +");
        case 0x4F:
            return QStringLiteral("小键盘 1");
        case 0x50:
            return QStringLiteral("小键盘 2");
        case 0x51:
            return QStringLiteral("小键盘 3");
        case 0x52:
            return QStringLiteral("小键盘 0");
        case 0x53:
            return QStringLiteral("小键盘 .");
        default:
            break;
        }
    }
    if (!extended && scanCode == 0x37) {
        return QStringLiteral("小键盘 *");
    }
    if (extended && scanCode == 0x35) {
        return QStringLiteral("小键盘 /");
    }
    if (extended && scanCode == 0x1C) {
        return QStringLiteral("小键盘 回车");
    }
    return {};
}

QString keyboardDisplayName(quint16 scanCode, bool extended, quint16 virtualKey)
{
    const QString numpad = numpadDisplayName(scanCode, extended);
    if (!numpad.isEmpty()) {
        return numpad;
    }

    if (virtualKey >= 'A' && virtualKey <= 'Z') {
        return QString(QChar(virtualKey));
    }
    if (virtualKey >= '0' && virtualKey <= '9') {
        return QString(QChar(virtualKey));
    }
    if (virtualKey >= VK_F1 && virtualKey <= VK_F24) {
        return QStringLiteral("F%1").arg(virtualKey - VK_F1 + 1);
    }

    switch (virtualKey) {
    case VK_LBUTTON:
        return QStringLiteral("鼠标左键");
    case VK_RBUTTON:
        return QStringLiteral("鼠标右键");
    case VK_MBUTTON:
        return QStringLiteral("鼠标中键");
    case VK_XBUTTON1:
        return QStringLiteral("鼠标侧键1");
    case VK_XBUTTON2:
        return QStringLiteral("鼠标侧键2");
    case VK_BACK:
        return QStringLiteral("退格");
    case VK_TAB:
        return QStringLiteral("Tab");
    case VK_CLEAR:
        return QStringLiteral("Clear");
    case VK_RETURN:
        return QStringLiteral("回车");
    case VK_PAUSE:
        return QStringLiteral("Pause");
    case VK_CAPITAL:
        return QStringLiteral("Caps Lock");
    case VK_ESCAPE:
        return QStringLiteral("Esc");
    case VK_SPACE:
        return QStringLiteral("空格");
    case VK_PRIOR:
        return QStringLiteral("Page Up");
    case VK_NEXT:
        return QStringLiteral("Page Down");
    case VK_END:
        return QStringLiteral("End");
    case VK_HOME:
        return QStringLiteral("Home");
    case VK_LEFT:
        return QStringLiteral("←");
    case VK_UP:
        return QStringLiteral("↑");
    case VK_RIGHT:
        return QStringLiteral("→");
    case VK_DOWN:
        return QStringLiteral("↓");
    case VK_SNAPSHOT:
        return QStringLiteral("Print Screen");
    case VK_INSERT:
        return QStringLiteral("Insert");
    case VK_DELETE:
        return QStringLiteral("Delete");
    case VK_LSHIFT:
        return QStringLiteral("左 Shift");
    case VK_RSHIFT:
        return QStringLiteral("右 Shift");
    case VK_LCONTROL:
        return QStringLiteral("左 Ctrl");
    case VK_RCONTROL:
        return QStringLiteral("右 Ctrl");
    case VK_LMENU:
        return QStringLiteral("左 Alt");
    case VK_RMENU:
        return QStringLiteral("右 Alt");
    case VK_LWIN:
        return QStringLiteral("左 Win");
    case VK_RWIN:
        return QStringLiteral("右 Win");
    case VK_APPS:
        return QStringLiteral("菜单键");
    case VK_NUMLOCK:
        return QStringLiteral("Num Lock");
    case VK_SCROLL:
        return QStringLiteral("Scroll Lock");
    case VK_OEM_PLUS:
        return QStringLiteral("=");
    case VK_OEM_MINUS:
        return QStringLiteral("-");
    case VK_OEM_COMMA:
        return QStringLiteral(",");
    case VK_OEM_PERIOD:
        return QStringLiteral(".");
    case VK_OEM_1:
        return QStringLiteral(";");
    case VK_OEM_2:
        return QStringLiteral("/");
    case VK_OEM_3:
        return QStringLiteral("`");
    case VK_OEM_4:
        return QStringLiteral("[");
    case VK_OEM_5:
        return QStringLiteral("\\");
    case VK_OEM_6:
        return QStringLiteral("]");
    case VK_OEM_7:
        return QStringLiteral("'");
    case VK_OEM_102:
        return QStringLiteral("\\");
    default:
        break;
    }

    wchar_t buffer[128] = {};
    const LONG keyData = (static_cast<LONG>(scanCode) << 16)
                         | (extended ? (1L << 24) : 0L);
    const int length = GetKeyNameTextW(keyData, buffer,
                                       static_cast<int>(std::size(buffer)));
    if (length > 0) {
        return QString::fromWCharArray(buffer, length);
    }
    return QStringLiteral("按键 0x%1")
        .arg(static_cast<uint>(virtualKey), 2, 16, QChar('0'))
        .toUpper();
}

KeyBinding mouseBinding(KeyBindingType type)
{
    KeyBinding binding;
    binding.type = type;
    return binding;
}

int jsonInteger(const QJsonValue& value, int fallback)
{
    if (!value.isDouble()) {
        return fallback;
    }
    return value.toInt(fallback);
}

}  // namespace

quint16 KeyBinding::canonicalVirtualKey(quint16 scanCode, bool extended,
                                        quint16 virtualKey)
{
    // A numpad key changes its VKey depending on NumLock. Normalize by scan code
    // so the same physical numpad key matches in both NumLock states.
    if (!extended) {
        switch (scanCode) {
        case 0x47:
            return VK_NUMPAD7;
        case 0x48:
            return VK_NUMPAD8;
        case 0x49:
            return VK_NUMPAD9;
        case 0x4A:
            return VK_SUBTRACT;
        case 0x4B:
            return VK_NUMPAD4;
        case 0x4C:
            return VK_NUMPAD5;
        case 0x4D:
            return VK_NUMPAD6;
        case 0x4E:
            return VK_ADD;
        case 0x4F:
            return VK_NUMPAD1;
        case 0x50:
            return VK_NUMPAD2;
        case 0x51:
            return VK_NUMPAD3;
        case 0x52:
            return VK_NUMPAD0;
        case 0x53:
            return VK_DECIMAL;
        case 0x37:
            return VK_MULTIPLY;
        default:
            break;
        }
    }

    if (extended && scanCode == 0x35) {
        return VK_DIVIDE;
    }
    if (extended && scanCode == 0x1C) {
        return VK_RETURN;
    }

    return virtualKey;
}

KeyBinding KeyBinding::fromRawKeyboard(quint16 scanCode, bool extended,
                                       quint16 virtualKey)
{
    KeyBinding binding;
    binding.type = KeyBindingType::Keyboard;
    binding.scanCode = scanCode;
    binding.extended = extended;
    binding.virtualKey = canonicalVirtualKey(scanCode, extended, virtualKey);
    return binding;
}

KeyBinding KeyBinding::mouseMiddle()
{
    return mouseBinding(KeyBindingType::MouseMiddle);
}

KeyBinding KeyBinding::mouseX1()
{
    return mouseBinding(KeyBindingType::MouseX1);
}

KeyBinding KeyBinding::mouseX2()
{
    return mouseBinding(KeyBindingType::MouseX2);
}

KeyBinding KeyBinding::wheelUp()
{
    return mouseBinding(KeyBindingType::WheelUp);
}

KeyBinding KeyBinding::wheelDown()
{
    return mouseBinding(KeyBindingType::WheelDown);
}

QString KeyBinding::displayName() const
{
    switch (type) {
    case KeyBindingType::None:
        return {};
    case KeyBindingType::Keyboard:
        return keyboardDisplayName(scanCode, extended, virtualKey);
    case KeyBindingType::MouseMiddle:
        return QStringLiteral("鼠标中键");
    case KeyBindingType::MouseX1:
        return QStringLiteral("鼠标侧键1");
    case KeyBindingType::MouseX2:
        return QStringLiteral("鼠标侧键2");
    case KeyBindingType::WheelUp:
        return QStringLiteral("滚轮上");
    case KeyBindingType::WheelDown:
        return QStringLiteral("滚轮下");
    }
    return {};
}

QJsonValue KeyBinding::toJsonValue() const
{
    if (type == KeyBindingType::None) {
        return QJsonValue(QJsonValue::Null);
    }

    if (type == KeyBindingType::Keyboard) {
        QJsonObject object;
        object.insert(QStringLiteral("type"), QStringLiteral("keyboard"));
        object.insert(QStringLiteral("scanCode"),
                      static_cast<int>(scanCode));
        object.insert(QStringLiteral("extended"), extended);
        object.insert(QStringLiteral("virtualKey"),
                      static_cast<int>(virtualKey));
        return object;
    }

    QString typeName;
    switch (type) {
    case KeyBindingType::MouseMiddle:
        typeName = QStringLiteral("mouseMiddle");
        break;
    case KeyBindingType::MouseX1:
        typeName = QStringLiteral("mouseX1");
        break;
    case KeyBindingType::MouseX2:
        typeName = QStringLiteral("mouseX2");
        break;
    case KeyBindingType::WheelUp:
        typeName = QStringLiteral("wheelUp");
        break;
    case KeyBindingType::WheelDown:
        typeName = QStringLiteral("wheelDown");
        break;
    case KeyBindingType::None:
    case KeyBindingType::Keyboard:
        break;
    }

    QJsonObject object;
    object.insert(QStringLiteral("type"), typeName);
    return object;
}

KeyBinding KeyBinding::fromJsonValue(const QJsonValue& value)
{
    if (!value.isObject()) {
        return {};
    }

    const QJsonObject object = value.toObject();
    const QString type = object.value(QStringLiteral("type")).toString();

    if (type == QStringLiteral("mouseMiddle")) {
        return mouseMiddle();
    }
    if (type == QStringLiteral("mouseX1")) {
        return mouseX1();
    }
    if (type == QStringLiteral("mouseX2")) {
        return mouseX2();
    }
    if (type == QStringLiteral("wheelUp")) {
        return wheelUp();
    }
    if (type == QStringLiteral("wheelDown")) {
        return wheelDown();
    }
    if (type != QStringLiteral("keyboard")) {
        return {};
    }

    const int scan = jsonInteger(object.value(QStringLiteral("scanCode")), -1);
    const int vkey = jsonInteger(object.value(QStringLiteral("virtualKey")), -1);
    if (scan < 0 || scan > 0xFFFF || vkey < 0 || vkey > 0xFFFF) {
        return {};
    }

    const bool extended = object.value(QStringLiteral("extended")).toBool(false);
    return fromRawKeyboard(static_cast<quint16>(scan), extended,
                           static_cast<quint16>(vkey));
}

bool KeyBinding::operator==(const KeyBinding& other) const
{
    if (type != other.type) {
        return false;
    }
    if (type == KeyBindingType::Keyboard) {
        return scanCode == other.scanCode && extended == other.extended
               && virtualKey == other.virtualKey;
    }
    return true;
}
