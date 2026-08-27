#pragma once

#include "ContainerID.h"
#include "Inventory.h"

class PlayerInventory {
public:
    struct SlotData {
        ContainerID containerID;
        int slot;
    };

    int getSelectedHotbarSlot();
    Inventory* getInventory();
};
