#pragma once

#include "KeyBinding.h"

#include <QJsonObject>

struct UserConfig {
    KeyBinding ToggleKey1;
    KeyBinding ToggleKey2;
    KeyBinding ResetKey;

    KeyBinding Weapon1Key1;
    KeyBinding Weapon1Key2;
    KeyBinding Weapon2Key1;
    KeyBinding Weapon2Key2;
    KeyBinding KnifeKey1;
    KeyBinding KnifeKey2;
    KeyBinding PistolKey1;
    KeyBinding PistolKey2;

    int Weapon1Capacity = 2;
    int Weapon2Capacity = 2;
    int PistolCapacity = 0;

    int DotSize = 3;
    int ScopeDelayMs = 0;

    KeyBinding CapacityIncKey;
    KeyBinding CapacityDecKey;

    static UserConfig defaults();

    QJsonObject toJson() const;
    static UserConfig fromJson(const QJsonObject& object);
};
