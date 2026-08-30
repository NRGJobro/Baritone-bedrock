#include "ItemStack.h"

#include "../Block/Block.h"

float ItemStack::getDestroySpeed(Block* block) {
    if (!isValid() || getItem() == nullptr || block == nullptr)
        return 0.f;
    return getItem()->getDestroySpeed(this, block);
}
