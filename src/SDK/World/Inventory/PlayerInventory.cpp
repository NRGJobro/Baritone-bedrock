#include "PlayerInventory.h"

int PlayerInventory::getSelectedHotbarSlot() {
    return hat::member_at<int>(this, 0x10);
}

Inventory* PlayerInventory::getInventory() {
    return hat::member_at<Inventory*>(this, 0xB8);
}
