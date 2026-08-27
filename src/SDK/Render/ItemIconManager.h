#pragma once

#include "../World/Item/ItemStack.h"
#include "TextureUVCoordinateSet.h"

class ItemIconManager {
public:
    static const TextureUVCoordinateSet& getIcon(const ItemStack& stack, int newAnimationFrame = 0, bool isInventoryPane = false);
};
