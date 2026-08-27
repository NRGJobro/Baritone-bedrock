#include "LevelChunk.h"

#include "../../../../Memory/Sig/SignatureManager.h"

std::atomic<ChunkState> LevelChunk::getLoadState() const { // +3: 0f b6 80 ? ? ? ? 90 3c ? 0f 84
    return hat::member_at<ChunkState>(this, 0xF8);
}

bool LevelChunk::isRedstoneLoaded() const { // +2: 38 90 04 11 00 00 74 49 // For more info: ChunksLoadedInfo::areAllChunksLoadedAndTicking // offset can be found in a function thats 2 layers deep inside of 48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 41 54 41 55 41 56 41 57 48 83 EC 30 FF
    return hat::member_at<bool>(this, 0x10F4);
}

std::array<ChunkLocalHeight, 256>& LevelChunk::getHeightmap() { // +5: 66 41 03 84 56
    return hat::member_at<std::array<ChunkLocalHeight, 256>>(this, 0xB98);
}

std::vector<SubChunk>& LevelChunk::getSubChunks() {
    return hat::member_at<std::vector<SubChunk>>(this, 0x148); // +3 : 48 8b 95 ? ? ? ? 0f b7 cb
}

glm::ivec3 LevelChunk::getTopRainBlockPos(const ChunkBlockPos& pos) const {
    static auto sig = GET_SIG("LevelChunk::getTopRainBlockPos");
    static auto func = *(decltype(&LevelChunk::getTopRainBlockPos)*)&sig;
    return (this->*func)(pos);
}
