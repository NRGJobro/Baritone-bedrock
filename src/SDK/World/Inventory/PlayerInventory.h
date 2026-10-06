#pragma once

class Inventory;

class PlayerInventory {
public:
    int getSelectedHotbarSlot();
    void setSelectedHotbarSlot(int slot);
    Inventory* getInventory();
};
