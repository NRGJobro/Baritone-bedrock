#include "BlockLegacy.h"

#include "../../../Utils/Utils.h"
#include "Material/Material.h"

int16_t BlockLegacy::getBlockId() {
    return hat::member_at<int16_t>(this, 0x17E);
}

const std::string& BlockLegacy::getName() const {
    return hat::member_at<const std::string>(this, 0x98);
}

bool BlockLegacy::isSolid() {
    // mSolid is bit 1 of the second packed flag byte in the current layout.
    return (hat::member_at<uint8_t>(this, 0x164) & 0x2) != 0;
}

mce::Color BlockLegacy::getMapColor(BlockSource* source, glm::ivec3 pos, Block* block) { // +3 (sig inside of MapItem::sampleMapData): 48 8B ? ? ? ? ? FF 15 ? ? ? ? 0F 10 ? ? E9
    mce::Color color{};
    return Utils::CallVFunc<140, mce::Color&, mce::Color&, BlockSource*, glm::ivec3&, Block*>(this, color, source, pos, block);
}

Material* BlockLegacy::getMaterial() {
    return hat::member_at<Material*>(this, 0x28);
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
