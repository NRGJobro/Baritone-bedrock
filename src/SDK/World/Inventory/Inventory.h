#pragma once

#include "../Item/ItemStack.h"

class Inventory {
public:
    ItemStack* getItem(int slot);
};
