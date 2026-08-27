#pragma once

class ChunkPos {
    glm::ivec2 value{};

public:
    ChunkPos() = default;

    explicit ChunkPos(const glm::ivec3& vec) {
        this->value.x = vec.x >> 4;
        this->value.y = vec.z >> 4;
    }

    ChunkPos(const int x, const int z) {
        this->value.x = x;
        this->value.y = z;
    }

    const glm::ivec2* operator->() const {
        return &this->value;
    }

    glm::ivec2* operator->() {
        return &this->value;
    }

    bool operator==(const ChunkPos& other) const {
        return this->value == other.value;
    }
};

template<>
struct std::hash<ChunkPos> {
    size_t operator()(const ChunkPos& obj) const noexcept {
        size_t seed = 0;

        seed ^= std::hash<int>{}(obj->x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        seed ^= std::hash<int>{}(obj->y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        return seed;
    }
};
