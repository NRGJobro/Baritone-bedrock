#include "Inventory.h"

#include "../../../Utils/Utils.h"

ItemStack* Inventory::getItem(const int slot) {
    return Utils::CallVFunc<7, ItemStack*, int>(this, slot);
}
