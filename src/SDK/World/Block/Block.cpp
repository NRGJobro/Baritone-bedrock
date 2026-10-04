#include "Block.h"

#include "../../../Memory/Sig/SignatureManager.h"
#include "../../../Utils/Utils.h"
#include "../Biome/biome_color_sampling/BiomeColorSampling.h"

BlockLegacy* Block::getBlockLegacy() {
    return hat::member_at<BlockLegacy*>(this, 0x68);
}

uint8_t Block::getLightEmission() {
    return hat::member_at<uint8_t>(this, 0x94);
}

BlockMapColorComponent* Block::getBlockMapColorComponent() {
    using func_t = BlockMapColorComponent*(*)(Block*);
    static auto func = Utils::getFromOffset<func_t>(GET_SIG("Block::getBlockMapColorComponent"), 1);
    return func(this);
}

mce::Color Block::getMapColor(BlockSource* region, const glm::ivec3& pos) {
    const auto mapColorComp = this->getBlockMapColorComponent();

    if (mapColorComp == nullptr)
        return this->getBlockLegacy()->getMapColor(region, pos, this);

    const auto policy = BiomeColorSampling::getMapPolicy(mapColorComp->tintMethod);

    if (policy == nullptr)
        return this->getBlockLegacy()->getMapColor(region, pos, this);

    return policy->get(region, pos) * mapColorComp->color;
}
