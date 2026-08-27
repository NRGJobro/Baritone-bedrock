#pragma once

#include "BlockPos.h"

#include <cstddef>

namespace baritone {

struct BlockState {
    bool loaded = true;
    bool solid = false;
    bool liquid = false;
    bool hazard = false;
};

class IWorld {
public:
    virtual ~IWorld() = default;
    [[nodiscard]] virtual BlockState getBlock(const BlockPos& pos) const = 0;
};

struct PathOptions {
    bool allowWater = true;
    bool allowDiagonal = true;
    bool allowAscend = true;
    bool allowFall = true;
    int maxFallHeight = 3;
    std::size_t maxExpandedNodes = 60000;
    std::size_t nodesPerTick = 350;
};

} // namespace baritone
