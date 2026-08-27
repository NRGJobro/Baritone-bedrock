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
    Swim,
    Parkour
};

struct Movement {
    BlockPos destination;
    MovementType type = MovementType::Traverse;
    double cost = 1.0;
};

class MovementGenerator {
public:
    [[nodiscard]] static std::vector<Movement> getMovements(const IWorld& world, const BlockPos& from, const PathOptions& options);
    [[nodiscard]] static bool canOccupy(const IWorld& world, const BlockPos& pos, const PathOptions& options);
    [[nodiscard]] static bool canStandAt(const IWorld& world, const BlockPos& pos, const PathOptions& options);
};

} // namespace baritone
