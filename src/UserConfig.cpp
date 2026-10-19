#include "UserConfig.h"

#include <algorithm>
#include <limits>

namespace {

KeyBinding keyboardBinding(quint16 scanCode, bool extended, quint16 virtualKey)
{
    return KeyBinding::fromRawKeyboard(scanCode, extended, virtualKey);
}

int boundedInteger(const QJsonObject& object, const QString& key, int fallback,
                   int minimum, int maximum)
{
    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        return fallback;
    }
    const int parsed = value.toInt(fallback);
    return std::clamp(parsed, minimum, maximum);
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
    return object;
}

UserConfig UserConfig::fromJson(const QJsonObject& object)
{
    UserConfig config = defaults();

    const auto readBinding = [&object](const QString& key,
                                       KeyBinding& target) {
        if (object.contains(key)) {
            target = KeyBinding::fromJsonValue(object.value(key));
        }
    };

    readBinding(QStringLiteral("toggleKey1"), config.ToggleKey1);
    readBinding(QStringLiteral("toggleKey2"), config.ToggleKey2);
    readBinding(QStringLiteral("resetKey"), config.ResetKey);

    readBinding(QStringLiteral("weapon1Key1"), config.Weapon1Key1);
    readBinding(QStringLiteral("weapon1Key2"), config.Weapon1Key2);
    readBinding(QStringLiteral("weapon2Key1"), config.Weapon2Key1);
    readBinding(QStringLiteral("weapon2Key2"), config.Weapon2Key2);
    readBinding(QStringLiteral("knifeKey1"), config.KnifeKey1);
    readBinding(QStringLiteral("knifeKey2"), config.KnifeKey2);
    readBinding(QStringLiteral("pistolKey1"), config.PistolKey1);
    readBinding(QStringLiteral("pistolKey2"), config.PistolKey2);

    config.Weapon1Capacity =
        boundedInteger(object, QStringLiteral("weapon1Capacity"),
                       config.Weapon1Capacity, 0, 9);
    config.Weapon2Capacity =
        boundedInteger(object, QStringLiteral("weapon2Capacity"),
                       config.Weapon2Capacity, 0, 9);
    config.PistolCapacity =
        boundedInteger(object, QStringLiteral("pistolCapacity"),
                       config.PistolCapacity, 0, 9);

    config.DotSize =
        boundedInteger(object, QStringLiteral("dotSize"), config.DotSize, 0,
                       std::numeric_limits<int>::max());
    config.ScopeDelayMs =
        boundedInteger(object, QStringLiteral("scopeDelayMs"),
                       config.ScopeDelayMs, 0, std::numeric_limits<int>::max());

    readBinding(QStringLiteral("capacityIncKey"), config.CapacityIncKey);
    readBinding(QStringLiteral("capacityDecKey"), config.CapacityDecKey);

    return config;
}
