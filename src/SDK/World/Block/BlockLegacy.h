#pragma once

#include "../../Client/mce/Color.h"
#include "../../Util/AABB.h"
#include "../Level/HitResult/FacingID.h"
#include "BlockSupportType.h"
#include "Material/Material.h"

class BlockSource;
class Block;

class BlockLegacy {
public:
    int16_t getBlockId();
    bool isSolid();
    mce::Color getMapColor(BlockSource* source, glm::ivec3 pos, Block* block);
    Material* getMaterial();


    const AABB& getVisualShape(Block* block, AABB& buffer);

    bool canProvideSupport(Block* block, FacingID facing = FacingID::Up, BlockSupportType supportType = BlockSupportType::Edge);

    bool isMultifaceBlock();

    bool mayPick(BlockSource* region, Block* block, bool liquid);
};
