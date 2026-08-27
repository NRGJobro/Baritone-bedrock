#pragma once

#include "Block/Block.h"
#include "Block/BlockVolume.h"
#include "Block/ShapeType.h"
#include "Level/Chunk/ChunkSource.h"
#include "Level/HitResult/HitResult.h"

class BlockSource {
public:
    class DefaultBlockScan { // Standard std::function doesn't work for some reason
        void* executionPtr;
        char pad[0x28]{};
        DefaultBlockScan* selfPtr;

        virtual void doNothing(void*) {}
        virtual void doNothing2(void*) {}
        virtual bool checkBlock(BlockSource* region, Block* block, bool liquid);

    public:
        DefaultBlockScan() {
            this->executionPtr = *reinterpret_cast<void**>(*reinterpret_cast<uintptr_t*>(this) + 0x10);
            this->selfPtr = this;
        }
    };

    ChunkSource* getChunkSource();
    int16_t getMinHeight();

    Block* getBlock(int x, int y, int z);
    Block* getBlock(const glm::ivec3& pos);

    HitResult clip(const glm::vec3& start, const glm::vec3& end, bool checkAgainstLiquid, ShapeType shapeType, int maxDistance, bool ignoreBorderBlocks, bool fullOnly, Actor* actor, const DefaultBlockScan& shouldCheckBlock, bool stopOnFirstLiquidHit);

    class Biome* getBiome(const glm::ivec3& pos);

    BrightnessPair getBrightnessPair(const glm::ivec3& pos);

    bool fetchBlocks(const glm::ivec3& origin, BlockVolume& volume);
};
