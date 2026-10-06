#pragma once

class ItemStack;

class Inventory {
public:
    ItemStack* getItem(int slot);
};
