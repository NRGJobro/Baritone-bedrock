#include "Item.h"

#include "ItemStackBase.h"
#include "../Block/Block.h"
#include "../../../Utils/Utils.h"

float Item::getDestroySpeed(ItemStackBase* stack, Block* block) {
    // Matching-version Borion Item vtable entry.
    return Utils::CallVFunc<83, float, ItemStackBase*, Block*>(this, stack, block);
}
