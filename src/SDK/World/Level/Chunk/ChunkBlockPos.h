#pragma once

struct ChunkBlockPos {
    int8_t x;
    int8_t z;
    int16_t y; // ChunkLocalHeight

    ChunkBlockPos(const glm::ivec3& blockPos, const int16_t minDimensionHeight) {
        this->x = static_cast<int8_t>(blockPos.x & 0xF);
        this->z = static_cast<int8_t>(blockPos.z & 0xF);
        this->y = static_cast<int16_t>(blockPos.y - minDimensionHeight);
    }

    ChunkBlockPos(const int x, const int z, const int16_t minDimensionHeight) {
        this->x = static_cast<int8_t>(x & 0xF);
        this->z = static_cast<int8_t>(z & 0xF);
        this->y = -minDimensionHeight;
    }

    explicit ChunkBlockPos(const glm::ivec3& blockPos) {
        this->x = static_cast<int8_t>(blockPos.x & 0xF);
        this->z = static_cast<int8_t>(blockPos.z & 0xF);
        this->y = static_cast<int16_t>(blockPos.y + 0x40);
    }

    [[nodiscard]] constexpr uint16_t toLegacyIndex() const noexcept { // e.g. for getting blocks from SubChunk
        return static_cast<uint16_t>(this->z) * 0x10 + (static_cast<uint16_t>(this->y) & 0xF | static_cast<uint16_t>(this->x) << 8);
    }
};
