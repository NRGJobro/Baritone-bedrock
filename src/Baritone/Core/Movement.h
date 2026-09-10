#pragma once

#include "World.h"

#include <vector>

namespace baritone {

enum class MovementType {
    Start,
    Traverse,
    Diagonal,
    Ascend,
    Descend,
    Fall,
    WaterDrop,
    Swim,
    Parkour,
    Bridge,
    BreakTraverse,
    BreakAscend,
    BreakDescend,
    BreakDown,
    BuildAscend
};

struct Movement {
    BlockPos destination;
    MovementType type = MovementType::Traverse;
    double cost = 1.0;
};

class MovementGenerator {
public:
    [[nodiscard]] static bool canDropToWater(const IWorld& world, const BlockPos& from, const BlockPos& to);
    [[nodiscard]] static bool isPlannedBreakCell(const BlockPos& from, const BlockPos& to,
        MovementType type, const BlockPos& cell);
    [[nodiscard]] static std::vector<Movement> getMovements(const IWorld& world, const BlockPos& from, const PathOptions& options);
    static void getMovements(const IWorld& world, const BlockPos& from,
        const PathOptions& options, std::vector<Movement>& result);
    [[nodiscard]] static bool canOccupy(const IWorld& world, const BlockPos& pos, const PathOptions& options);
    [[nodiscard]] static bool canStandAt(const IWorld& world, const BlockPos& pos, const PathOptions& options);
    // Removing a solid cell lets liquid above or on any horizontal side flow
    // into the newly opened tunnel. Liquid below cannot flow upward.
    // Unknown neighbors are unsafe until loaded and checked as well.
    [[nodiscard]] static bool wouldExposeLiquid(const IWorld& world, const BlockPos& pos);
};

} // namespace baritone
