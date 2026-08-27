#pragma once

#include "../Inventory/PlayerInventory.h"
#include "Mob.h"

class Player : public Mob {
public:
    PlayerInventory* getSupplies();
};
