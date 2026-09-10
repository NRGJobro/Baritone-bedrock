#pragma once

#include "BlockPos.h"

#include <cstddef>

namespace baritone {

struct BlockState {
    bool loaded = true;
    bool solid = false;
    bool liquid = false;
    bool hazard = false;
    bool breakable = false;
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
    bool allowParkour = true;
    bool allowParkourAscend = true;
    bool allowBridge = false;
    // Restrict construction to flat cardinal crossings whose missing support
    // cells contain non-hazardous liquid. Mining enables this so it may bridge
    // water without considering arbitrary scaffolding routes over dry gaps.
    bool bridgeOverWaterOnly = false;
    // Mining enables this directly. The controller enables it for ordinary
    // navigation only after a bounded search without terrain changes fails.
    bool allowBreak = false;
    // Mining uses a bounded local search and should not expand into the large
    // bridge fallback budget used by ordinary navigation.
    bool miningMode = false;
    // First search without construction; if that search exhausts, the
    // controller retries with bridge transitions enabled.
    bool bridgeOnlyAfterFailure = true;
    bool preferSprint = true;
    int maxParkourDistance = 4;
    int maxFallHeight = 3;
    int maxBridgeLength = 8;
    // A directed bounded search is preferable on Bedrock's game thread. Long
    // goals continue through partial paths as chunks load instead of scanning
    // every reachable cell in the current chunk set at once.
    std::size_t maxExpandedNodes = 24000;
    std::size_t nodesPerTick = 160;
    // Values above one make A* greedier. Mining temporarily raises this so it
    // tunnels toward nearby ore instead of exhaustively exploring old caves.
    double heuristicWeight = 1.55;
};

} // namespace baritone
