#pragma once

#include "../../../Util/SpinLockImpl.h"
#include "../../Block/Block.h"
#include "../BrightnessPair.h"
#include "DirtyTicksCounter.h"
#include "SubChunkBrightnessStorage.h"
#include "SubChunkStorage.h"

struct SubChunk {
    using Layer = int8_t;

    enum class BlockLayer : int8_t {
        Standard,
        Extra,
        Count
    };

    enum class SubChunkState : int {
        Invalid = -1,
        Normal = 0,
        IsLightingSystemSubChunk = 1,
        NeedsRequest = 2,
        ReceivedResponseFromServer = 3,
        ProcessingSubChunk = 4,
        WaitingForCacheResponse = 5,
        ProcessedSubChunk = 6,
        RequestFinished = 7
    };

    DirtyTicksCounter dirtyTicksCounter;
    std::unique_ptr<SubChunkBrightnessStorage> skyLight;
    std::unique_ptr<SubChunkBrightnessStorage> blockLight;
    SubChunkState subChunkState;
    bool hasMaxSkyLight;
    bool needsInitLighting;
    bool needsClientLighting;
    std::unique_ptr<SubChunkStorage<Block>> blocks[2];
    SubChunkStorage<Block>* blocksReadPtr[2];
    SpinLockImpl* writeLock;
    uint64_t hash;
    bool hashDirty;
    char absoluteIndex;
    bool isReplacementSubChunk;
    unsigned char renderChunkTrackingVersionNumber;

    BrightnessPair getLightLevelAt(glm::ivec3 localPos) const;
};
