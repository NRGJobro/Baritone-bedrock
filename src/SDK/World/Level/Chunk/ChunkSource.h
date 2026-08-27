#pragma once

#include "ChunkPos.h"
#include "LevelChunk.h"

class ChunkSource {
public:
    std::unordered_map<ChunkPos, std::weak_ptr<LevelChunk>>& getChunkStorage();
    LevelChunk* getAvailableChunk(ChunkPos pos);
};
