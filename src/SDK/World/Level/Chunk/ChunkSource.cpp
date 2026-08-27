#include "ChunkSource.h"
#include "LevelChunk.h"

std::unordered_map<ChunkPos, std::weak_ptr<LevelChunk>>& ChunkSource::getChunkStorage() {
    return hat::member_at<std::unordered_map<ChunkPos, std::weak_ptr<LevelChunk>>>(this, 0x70); // Apparently its somewhere here, but it looks fake: +3 - 49 8B 81 88 00 00 00 48 03 D1
}

LevelChunk* ChunkSource::getAvailableChunk(const ChunkPos pos) {
    auto& storage = getChunkStorage();

    auto it = storage.find(pos);

    if (it != storage.end()) {
        if (const auto chunk = it->second.lock()) {
            if (chunk->isFullyLoaded())
                return chunk.get();

            return nullptr;
        }
    }

    return nullptr;
}
