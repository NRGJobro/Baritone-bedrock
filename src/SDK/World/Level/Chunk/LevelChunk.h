#pragma once

#include "ChunkBlockPos.h"
#include "ChunkLocalHeight.h"
#include "ChunkState.h"
#include "SubChunk.h"

class LevelChunk {
public:
    [[nodiscard]] std::atomic<ChunkState> getLoadState() const;
    [[nodiscard]] bool isRedstoneLoaded() const;
    std::array<ChunkLocalHeight, 256>& getHeightmap();
    std::vector<SubChunk>& getSubChunks();
    [[nodiscard]] glm::ivec3 getTopRainBlockPos(const ChunkBlockPos& pos) const;

    [[nodiscard]] bool isFullyLoaded() const {
        return getLoadState() == ChunkState::Loaded && isRedstoneLoaded();
    }
};
