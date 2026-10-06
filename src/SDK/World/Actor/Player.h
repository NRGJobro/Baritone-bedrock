#pragma once

#include "Mob.h"

class PlayerInventory;

class Player : public Mob {
public:
    PlayerInventory* getSupplies();
};
