#pragma once

struct RuntimeState {
    // 0 = Weapon 1, 1 = Weapon 2, 2 = Knife, 3 = Pistol.
    int CurrentWeapon = 0;

    int Weapon1AmmoState = 0;
    int Weapon2AmmoState = 0;
    int PistolAmmoState = 0;

    int RuntimeWeapon1Capacity = 2;
    int RuntimeWeapon2Capacity = 2;
    int RuntimePistolCapacity = 0;
};
