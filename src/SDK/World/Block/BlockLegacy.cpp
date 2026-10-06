#include "BlockLegacy.h"

#include "Material/Material.h"

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

Material* BlockLegacy::getMaterial() {
    return hat::member_at<Material*>(this, 0x28);
}
