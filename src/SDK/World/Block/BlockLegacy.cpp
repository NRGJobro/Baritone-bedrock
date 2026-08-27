#include "BlockLegacy.h"

#include "../../../Utils/Utils.h"
#include "Material/Material.h"

int16_t BlockLegacy::getBlockId() {
    return hat::member_at<int16_t>(this, 0x1AA);
}

bool BlockLegacy::isSolid() {
    return hat::member_at<bool>(this, 0x17C);
}

mce::Color BlockLegacy::getMapColor(BlockSource* source, glm::ivec3 pos, Block* block) { // +3 (sig inside of MapItem::sampleMapData): 48 8B ? ? ? ? ? FF 15 ? ? ? ? 0F 10 ? ? E9
    mce::Color color{};
    return Utils::CallVFunc<140, mce::Color&, mce::Color&, BlockSource*, glm::ivec3&, Block*>(this, color, source, pos, block);
}

Material* BlockLegacy::getMaterial() {
    return hat::member_at<Material*>(this, 0x140); // +3 (in MapItem::sampleMapData): 48 8B ? ? ? ? ? 44 8B ? ? ? 83 38
}

const AABB& BlockLegacy::getVisualShape(Block* block, AABB& buffer) {
    return Utils::CallVFunc<11, const AABB&, Block*, AABB&>(this, block, buffer);
}

bool BlockLegacy::canProvideSupport(Block* block, FacingID facing, BlockSupportType supportType) {
    return Utils::CallVFunc<22, bool, Block*, FacingID, BlockSupportType>(this, block, facing, supportType);
}

bool BlockLegacy::isMultifaceBlock() {
    return Utils::CallVFunc<43, bool>(this);
}

bool BlockLegacy::mayPick(BlockSource* region, Block* block, const bool liquid) {
    return Utils::CallVFunc<76, bool, BlockSource*, Block*, bool>(this, region, block, liquid);
}
