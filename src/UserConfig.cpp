#include "UserConfig.h"

#include <cmath>
#include <limits>

namespace {

KeyBinding keyboardBinding(quint16 scanCode, bool extended, quint16 virtualKey)
{
    return KeyBinding::fromRawKeyboard(scanCode, extended, virtualKey);
}

std::optional<int> jsonInteger(const QJsonValue& value, int minimum,
                               int maximum)
{
    if (!value.isDouble()) {
        return std::nullopt;
    }

    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number
        || number < minimum || number > maximum) {
        return std::nullopt;
    }
    return static_cast<int>(number);
}

bool readInteger(const QJsonObject& object, const QString& key, int minimum,
                 int maximum, int* target)
{
    const std::optional<int> value =
        jsonInteger(object.value(key), minimum, maximum);
    if (!value.has_value()) {
        return false;
    }
    *target = *value;
    return true;
}

bool readBool(const QJsonObject& object, const QString& key, bool* target)
{
    const QJsonValue value = object.value(key);
    if (!value.isBool()) {
        return false;
    }
    *target = value.toBool();
    return true;
}

bool readScale(const QJsonObject& object, const QString& key, double* target)
{
    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        return false;
    }

    const double number = value.toDouble();
    if (!std::isfinite(number) || number < 0.5 || number > 3.0) {
        return false;
    }

    const double tenths = number * 10.0;
    const double rounded = std::round(tenths);
    if (std::abs(tenths - rounded) > 1e-6) {
        return false;
    }

    *target = rounded / 10.0;
    return true;
}

bool readBinding(const QJsonObject& object, const QString& key,
                 KeyBinding* target)
{
    const QJsonValue value = object.value(key);
    if (value.isNull()) {
        *target = {};
        return true;
    }
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject bindingObject = value.toObject();
    const QString type =
        bindingObject.value(QStringLiteral("type")).toString();

    if (type == QStringLiteral("mouseMiddle")) {
        *target = KeyBinding::mouseMiddle();
        return true;
    }
    if (type == QStringLiteral("mouseX1")) {
        *target = KeyBinding::mouseX1();
        return true;
    }
    if (type == QStringLiteral("mouseX2")) {
        *target = KeyBinding::mouseX2();
        return true;
    }
    if (type == QStringLiteral("wheelUp")) {
        *target = KeyBinding::wheelUp();
        return true;
    }
    if (type == QStringLiteral("wheelDown")) {
        *target = KeyBinding::wheelDown();
        return true;
    }
    if (type != QStringLiteral("keyboard")) {
        return false;
    }

    const std::optional<int> scanCode =
        jsonInteger(bindingObject.value(QStringLiteral("scanCode")), 0, 0xFFFF);
    const std::optional<int> virtualKey = jsonInteger(
        bindingObject.value(QStringLiteral("virtualKey")), 0, 0xFFFF);
    const QJsonValue extendedValue =
        bindingObject.value(QStringLiteral("extended"));
    if (!scanCode.has_value() || !virtualKey.has_value()
        || !extendedValue.isBool()) {
        return false;
    }

    *target = KeyBinding::fromRawKeyboard(
        static_cast<quint16>(*scanCode), extendedValue.toBool(),
        static_cast<quint16>(*virtualKey));
    return true;
}

}  // namespace

UserConfig UserConfig::defaults()
{
    UserConfig config;

    config.ToggleKey1 = keyboardBinding(0x30, false, 'B');
    config.ToggleKey2 = keyboardBinding(0x0B, false, '0');
    config.ResetKey = keyboardBinding(0x0D, false, 0xBB);  // VK_OEM_PLUS ("=")

    config.Weapon1Key1 = keyboardBinding(0x02, false, '1');
    config.Weapon1Key2 = {};
    config.Weapon2Key1 = keyboardBinding(0x03, false, '2');
    config.Weapon2Key2 = {};
    config.KnifeKey1 = keyboardBinding(0x04, false, '3');
    config.KnifeKey2 = {};
    config.PistolKey1 = keyboardBinding(0x05, false, '4');
    config.PistolKey2 = {};

    config.Weapon1Capacity = 2;
    config.Weapon2Capacity = 2;
    config.PistolCapacity = 0;

    config.DotSize = 3;
    config.ScopeDelayMs = 0;

    config.CapacityIncKey = keyboardBinding(0x48, true, 0x26);  // VK_UP
    config.CapacityDecKey = keyboardBinding(0x50, true, 0x28);  // VK_DOWN

    return config;
}

QJsonObject UserConfig::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("toggleKey1"), ToggleKey1.toJsonValue());
    object.insert(QStringLiteral("toggleKey2"), ToggleKey2.toJsonValue());
    object.insert(QStringLiteral("resetKey"), ResetKey.toJsonValue());

    object.insert(QStringLiteral("weapon1Key1"), Weapon1Key1.toJsonValue());
    object.insert(QStringLiteral("weapon1Key2"), Weapon1Key2.toJsonValue());
    object.insert(QStringLiteral("weapon2Key1"), Weapon2Key1.toJsonValue());
    object.insert(QStringLiteral("weapon2Key2"), Weapon2Key2.toJsonValue());
    object.insert(QStringLiteral("knifeKey1"), KnifeKey1.toJsonValue());
    object.insert(QStringLiteral("knifeKey2"), KnifeKey2.toJsonValue());
    object.insert(QStringLiteral("pistolKey1"), PistolKey1.toJsonValue());
    object.insert(QStringLiteral("pistolKey2"), PistolKey2.toJsonValue());

    object.insert(QStringLiteral("weapon1Capacity"), Weapon1Capacity);
    object.insert(QStringLiteral("weapon2Capacity"), Weapon2Capacity);
    object.insert(QStringLiteral("pistolCapacity"), PistolCapacity);

    object.insert(QStringLiteral("dotSize"), DotSize);
    object.insert(QStringLiteral("scopeDelayMs"), ScopeDelayMs);

    object.insert(QStringLiteral("capacityIncKey"),
                  CapacityIncKey.toJsonValue());
    object.insert(QStringLiteral("capacityDecKey"),
                  CapacityDecKey.toJsonValue());

    object.insert(QStringLiteral("statusOverlayEnabled"), StatusOverlayEnabled);
    object.insert(QStringLiteral("statusOverlayX"), StatusOverlayX);
    object.insert(QStringLiteral("statusOverlayY"), StatusOverlayY);
    object.insert(QStringLiteral("statusOverlayScale"), StatusOverlayScale);
    return object;
}

std::optional<UserConfig> UserConfig::fromJson(const QJsonObject& object)
{
    UserConfig config = defaults();

    if (!readBinding(object, QStringLiteral("toggleKey1"), &config.ToggleKey1)
        || !readBinding(object, QStringLiteral("toggleKey2"),
                        &config.ToggleKey2)
        || !readBinding(object, QStringLiteral("resetKey"), &config.ResetKey)
        || !readBinding(object, QStringLiteral("weapon1Key1"),
                        &config.Weapon1Key1)
        || !readBinding(object, QStringLiteral("weapon1Key2"),
                        &config.Weapon1Key2)
        || !readBinding(object, QStringLiteral("weapon2Key1"),
                        &config.Weapon2Key1)
        || !readBinding(object, QStringLiteral("weapon2Key2"),
                        &config.Weapon2Key2)
        || !readBinding(object, QStringLiteral("knifeKey1"),
                        &config.KnifeKey1)
        || !readBinding(object, QStringLiteral("knifeKey2"),
                        &config.KnifeKey2)
        || !readBinding(object, QStringLiteral("pistolKey1"),
                        &config.PistolKey1)
        || !readBinding(object, QStringLiteral("pistolKey2"),
                        &config.PistolKey2)
        || !readBinding(object, QStringLiteral("capacityIncKey"),
                        &config.CapacityIncKey)
        || !readBinding(object, QStringLiteral("capacityDecKey"),
                        &config.CapacityDecKey)) {
        return std::nullopt;
    }

    if (!readInteger(object, QStringLiteral("weapon1Capacity"), 0, 9,
                     &config.Weapon1Capacity)
        || !readInteger(object, QStringLiteral("weapon2Capacity"), 0, 9,
                        &config.Weapon2Capacity)
        || !readInteger(object, QStringLiteral("pistolCapacity"), 0, 9,
                        &config.PistolCapacity)
        || !readInteger(object, QStringLiteral("dotSize"), 0,
                        std::numeric_limits<int>::max(), &config.DotSize)
        || !readInteger(object, QStringLiteral("scopeDelayMs"), 0,
                        std::numeric_limits<int>::max(),
                        &config.ScopeDelayMs)
        || !readBool(object, QStringLiteral("statusOverlayEnabled"),
                     &config.StatusOverlayEnabled)
        || !readInteger(object, QStringLiteral("statusOverlayX"),
                        std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max(),
                        &config.StatusOverlayX)
        || !readInteger(object, QStringLiteral("statusOverlayY"),
                        std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max(),
                        &config.StatusOverlayY)
        || !readScale(object, QStringLiteral("statusOverlayScale"),
                      &config.StatusOverlayScale)) {
        return std::nullopt;
    }

    return config;
}
