#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace baritone {

struct BlockPos {
    int x{};
    int y{};
    int z{};

    constexpr BlockPos offset(const int dx, const int dy, const int dz) const {
        return {x + dx, y + dy, z + dz};
    }

    constexpr bool operator==(const BlockPos&) const = default;
};

struct BlockPosHash {
    std::size_t operator()(const BlockPos& pos) const noexcept {
        std::size_t seed = std::hash<int>{}(pos.x);
        seed ^= std::hash<int>{}(pos.y) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
        seed ^= std::hash<int>{}(pos.z) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
        return seed;
    }
};

} // namespace baritone
