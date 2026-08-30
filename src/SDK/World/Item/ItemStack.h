#pragma once

#include "ItemStackBase.h"

class ItemStack : public ItemStackBase {
public:
    float getDestroySpeed(class Block* block);
};
