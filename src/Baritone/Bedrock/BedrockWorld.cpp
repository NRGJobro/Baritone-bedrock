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

    std::string_view blockName = legacy->getName();
    if (blockName.starts_with("minecraft:"))
        blockName.remove_prefix(10);
    // A few damage/debuff blocks use generic Plant or Solid materials, so the
    // material flags alone do not identify them reliably across Bedrock
    // versions.
    const bool nameHazard = blockName == "sweet_berry_bush" || blockName == "wither_rose" ||
        blockName == "cactus" || blockName == "fire" || blockName == "soul_fire" ||
        blockName == "campfire" || blockName == "soul_campfire" ||
        blockName == "magma_block" || blockName == "pointed_dripstone";
    const bool hazard = material->superHot || material->type == MaterialType::Lava ||
        material->type == MaterialType::Fire || material->type == MaterialType::Cactus ||
        material->type == MaterialType::PowderSnow || nameHazard;

    // Bedrock's generic material flags do not line up perfectly with player
    // collision. Full tree leaves can report a non-solid Leaves material even
    // though the player cannot walk through the canopy. Conversely, leaf
    // litter and ordinary plants may carry a blocks-motion/solid flag while
    // having no collision that obstructs movement.
    const bool groundLeafLitter = blockName == "leaf_litter";
    const bool treeLeaves = !groundLeafLitter &&
        (material->type == MaterialType::Leaves || blockName == "leaves" ||
            blockName == "leaves2" || blockName.ends_with("_leaves"));
    // Cobwebs are plant-like/non-solid in some versions but are intentionally
    // not clearance: routing through one can slow the player until the normal
    // executor declares itself stuck.
    const bool movementTrap = blockName == "web" || blockName == "cobweb";
    const bool passableVegetation = !hazard && !movementTrap && !treeLeaves &&
        (groundLeafLitter || material->type == MaterialType::Plant ||
            material->type == MaterialType::NonSolid);
    // Legacy numeric IDs beyond air are not stable across Bedrock versions.
    // Treating IDs 210/217/416 as special caused ordinary deepslate-era blocks
    // to invalidate mining routes immediately after a neighbor was removed.
    const bool unbreakableByName = blockName == "bedrock" || blockName == "invisible_bedrock" ||
        blockName == "barrier" ||
        blockName == "structure_void" || blockName == "end_portal_frame" ||
        blockName == "reinforced_deepslate" || blockName == "border_block" ||
        blockName == "allow" || blockName == "deny";
    // Bedrock's legacy ID 7 is stable and provides a defensive fallback when a
    // server/resource pack exposes an unexpected name.
    const bool unbreakable = legacy->getBlockId() == 7 || unbreakableByName ||
        material->type == MaterialType::Barrier ||
        material->type == MaterialType::StructureVoid;
    const bool reportedSolid = legacy->isSolid() || material->solid || material->blocksMotion;
    const bool solid = treeLeaves || movementTrap || (reportedSolid && !passableVegetation);

    return {
        .loaded = true,
        .solid = solid,
        .liquid = material->liquid,
        .hazard = hazard,
        .breakable = solid && !unbreakable && !material->liquid && !hazard
    };
}

} // namespace baritone
