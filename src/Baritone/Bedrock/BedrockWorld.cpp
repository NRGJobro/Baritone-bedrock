#include "BedrockWorld.h"

#include "../../SDK/World/Block/Material/MaterialType.h"
#include "../../SDK/World/BlockSource.h"

namespace baritone {

BedrockWorld::BedrockWorld(BlockSource* region) : region(region) {}

BlockState BedrockWorld::getBlock(const BlockPos& pos) const {
    // Avoid version-fragile BlockSource height offsets. Every currently
    // supported Bedrock dimension is safely contained by these guard rails.
    if (region == nullptr || pos.y < -128 || pos.y > 512)
        return {.loaded = false};

    const auto block = region->getBlock(pos.x, pos.y, pos.z);
    if (block == nullptr)
        return {.loaded = false};

    const auto legacy = block->getBlockLegacy();
    if (legacy == nullptr)
        return {.loaded = false};

    const auto material = legacy->getMaterial();
    if (material == nullptr)
        return {.loaded = false};

    // ChunkSource::getChunkStorage is intentionally not used here. Its offset is
    // unreliable on multiplayer clients and made every block look unloaded.
    if (material->type == MaterialType::ClientRequestPlaceholder)
        return {.loaded = false};

    if (legacy->getBlockId() == 0 || material->type == MaterialType::Air)
        return {.loaded = true};

    const bool hazard = material->superHot || material->type == MaterialType::Lava || material->type == MaterialType::Fire ||
        material->type == MaterialType::Cactus || material->type == MaterialType::PowderSnow;

    return {
        .loaded = true,
        .solid = legacy->isSolid() || material->solid || material->blocksMotion,
        .liquid = material->liquid,
        .hazard = hazard
    };
}

} // namespace baritone
