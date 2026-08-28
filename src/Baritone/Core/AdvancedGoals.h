#pragma once

#include "Goal.h"

#include <cstdlib>

namespace baritone {

// Java Baritone GoalGetToBlock: stop adjacent to the requested block rather
// than entering it. Useful for chests, machines, and interaction targets.
class GoalGetToBlock final : public Goal {
    BlockPos target;
public:
    explicit GoalGetToBlock(BlockPos target) : target(target) {}
    [[nodiscard]] bool isInGoal(const BlockPos& pos) const override {
        const int dx = pos.x - target.x;
        const int dy = pos.y - target.y;
        const int dz = pos.z - target.z;
        return std::abs(dx) + std::abs(dy < 0 ? dy + 1 : dy) + std::abs(dz) <= 1;
    }
    [[nodiscard]] double heuristic(const BlockPos& pos) const override {
        const int dy = pos.y - target.y;
        return std::abs(pos.x - target.x) + std::abs(dy < 0 ? dy + 1 : dy) + std::abs(pos.z - target.z);
    }
    [[nodiscard]] std::string describe() const override { return "interact " + std::to_string(target.x) + " " + std::to_string(target.y) + " " + std::to_string(target.z); }
    [[nodiscard]] const BlockPos& getTarget() const { return target; }
};

// Java Baritone GoalTwoBlocks: accept either the requested block or the block
// immediately below it, matching the two-block interaction clearance goal.
class GoalTwoBlocks final : public Goal {
    BlockPos target;
public:
    explicit GoalTwoBlocks(BlockPos target) : target(target) {}
    [[nodiscard]] bool isInGoal(const BlockPos& pos) const override {
        return pos.x == target.x && pos.z == target.z && (pos.y == target.y || pos.y == target.y - 1);
    }
    [[nodiscard]] double heuristic(const BlockPos& pos) const override {
        const int dy = pos.y - target.y;
        return std::abs(pos.x - target.x) + std::abs(dy < 0 ? dy + 1 : dy) + std::abs(pos.z - target.z);
    }
    [[nodiscard]] std::string describe() const override { return "two-block " + std::to_string(target.x) + " " + std::to_string(target.y) + " " + std::to_string(target.z); }
    [[nodiscard]] const BlockPos& getTarget() const { return target; }
};

} // namespace baritone
