#pragma once

#include "BlockLegacy.h"
#include "Components/BlockMapColorComponent.h"

class Block {
public:
    BlockLegacy* getBlockLegacy();
    uint8_t getLightEmission();

    BlockMapColorComponent* getBlockMapColorComponent();

    mce::Color getMapColor(BlockSource* region, const glm::ivec3& pos);
};
