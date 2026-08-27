#include "BlockSource.h"

#include "../../Memory/Sig/SignatureManager.h"
#include "../../Utils/Utils.h"
#include "../MC.h"
#include "Block/Material/MaterialType.h"
#include "Level/Chunk/ChunkBlockPos.h"
#include "Level/Chunk/ChunkPos.h"

bool BlockSource::DefaultBlockScan::checkBlock(BlockSource* region, Block* block, const bool liquid) {
    const auto blockLegacy = block->getBlockLegacy();

    if (blockLegacy->getBlockId() == 0)
        return false;

    const auto material = blockLegacy->getMaterial();

    if (material->type == MaterialType::Leaves || material->type == MaterialType::Plant || material->type == MaterialType::SolidPlant || material->type == MaterialType::Fire ||
        material->type == MaterialType::Glass || material->type == MaterialType::Portal || material->type == MaterialType::Bubble || material->type == MaterialType::Barrier ||
        material->type == MaterialType::DecorationSolid || material->type == MaterialType::NonSolid || !material->solid)
        return false;

    if (blockLegacy->isMultifaceBlock())
        return false;

    AABB aabb{};
    blockLegacy->getVisualShape(block, aabb);

    if (aabb.lower != aabb.upper) {
        if (aabb.upper.x - aabb.lower.x < 0.7f || aabb.upper.y - aabb.lower.y < 0.7f || aabb.upper.z - aabb.lower.z < 0.7f)
            return false;
    }

    return blockLegacy->mayPick(region, block, liquid);
}

ChunkSource* BlockSource::getChunkSource() {
    return hat::member_at<ChunkSource*>(this, 0x28);
}

int16_t BlockSource::getMinHeight() {
    return hat::member_at<int16_t>(this, 0x3A);
}

Block* BlockSource::getBlock(int x, int y, int z) {
    return this->getBlock({x, y, z});
}

Block* BlockSource::getBlock(const glm::ivec3& pos) {
    return Utils::CallVFunc<2, Block*, const glm::ivec3&>(this, pos);
}

HitResult BlockSource::clip(const glm::vec3& start, const glm::vec3& end, const bool checkAgainstLiquid, const ShapeType shapeType, const int maxDistance, const bool ignoreBorderBlocks, const bool fullOnly, Actor* actor, const DefaultBlockScan& shouldCheckBlock, const bool stopOnFirstLiquidHit) {
    const auto vtable = *reinterpret_cast<uintptr_t***>(this);
    static auto func = *(decltype(&BlockSource::clip)*)&vtable[50];
    return (this->*func)(start, end, checkAgainstLiquid, shapeType, maxDistance, ignoreBorderBlocks, fullOnly, actor, shouldCheckBlock, stopOnFirstLiquidHit);
}

Biome* BlockSource::getBiome(const glm::ivec3& pos) {
    using func_t = Biome*(*)(BlockSource*, const glm::ivec3&);
    static auto func = Utils::getFromOffset<func_t>(GET_SIG("BlockSource::getBiome"), 1);
    return func(this, pos);
}

BrightnessPair BlockSource::getBrightnessPair(const glm::ivec3& pos) {
    const auto chunk = this->getChunkSource()->getAvailableChunk(ChunkPos{pos});

    if (chunk == nullptr)
        return {};

    ChunkBlockPos cbp{pos, MC::getLocalPlayer()->getDimension()->getMinHeight()};

    const auto subChunkIndex = (cbp.y + 1) / 16;

    if (subChunkIndex < 0 || subChunkIndex >= chunk->getSubChunks().size())
        return {};

    const auto& subChunk = chunk->getSubChunks()[subChunkIndex];

    const glm::ivec3 lightPos{cbp.x, (pos.y - MC::getLocalPlayer()->getDimension()->getMinHeight() + 1) & 0xF, cbp.z};

    return subChunk.getLightLevelAt(lightPos);
}

bool BlockSource::fetchBlocks(const glm::ivec3& origin, BlockVolume& volume) {
    static auto sig = GET_SIG("BlockSource::fetchBlocks");
    using func_t = bool(*)(BlockSource*, const glm::ivec3&, BlockVolume&);
    static auto func = reinterpret_cast<func_t>(sig);
    return func(this, origin, volume);
}
