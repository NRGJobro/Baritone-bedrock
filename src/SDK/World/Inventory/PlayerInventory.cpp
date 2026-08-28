#include "PlayerInventory.h"

int PlayerInventory::getSelectedHotbarSlot() {
    return hat::member_at<int>(this, 0x10);
}

void PlayerInventory::setSelectedHotbarSlot(const int slot) {
    hat::member_at<int>(this, 0x10) = std::clamp(slot, 0, 8);
}

Inventory* PlayerInventory::getInventory() {
    return hat::member_at<Inventory*>(this, 0xB8);
}
