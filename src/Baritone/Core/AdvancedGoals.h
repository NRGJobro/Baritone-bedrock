#pragma once

#include "Goal.h"
#include "ActionCosts.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

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
        return (std::abs(pos.x - target.x) + std::abs(dy < 0 ? dy + 1 : dy) +
            std::abs(pos.z - target.z)) * action_costs::sprintOneBlock;
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
        return (std::abs(pos.x - target.x) + std::abs(dy < 0 ? dy + 1 : dy) +
            std::abs(pos.z - target.z)) * action_costs::sprintOneBlock;
    }
    [[nodiscard]] std::string describe() const override { return "two-block " + std::to_string(target.x) + " " + std::to_string(target.y) + " " + std::to_string(target.z); }
    [[nodiscard]] const BlockPos& getTarget() const { return target; }
};

// Matches Java Baritone's GoalThreeBlocks. Mining may finish with the
// player's feet in the target column, one block below it, or two blocks below
// it, allowing vertically connected ore veins to share one path calculation.
class GoalThreeBlocks final : public Goal {
    BlockPos target;
public:
    explicit GoalThreeBlocks(BlockPos target) : target(target) {}
    [[nodiscard]] bool isInGoal(const BlockPos& pos) const override {
        return pos.x == target.x && pos.z == target.z &&
            (pos.y == target.y || pos.y == target.y - 1 || pos.y == target.y - 2);
    }
    [[nodiscard]] double heuristic(const BlockPos& pos) const override {
        const int dy = pos.y - target.y;
        const int adjustedY = dy < -1 ? dy + 2 : (dy == -1 ? 0 : dy);
        return (std::abs(pos.x - target.x) + std::abs(adjustedY) +
            std::abs(pos.z - target.z)) * action_costs::sprintOneBlock;
    }
    [[nodiscard]] std::string describe() const override {
        return "three-block " + std::to_string(target.x) + " " +
            std::to_string(target.y) + " " + std::to_string(target.z);
    }
};

// Head toward the nearer overworld axis (X=0 or Z=0), as Baritone's
// `axis` / `highway` command does.
class GoalAxis final : public Goal {
public:
    [[nodiscard]] bool isInGoal(const BlockPos& pos) const override {
        return pos.x == 0 || pos.z == 0;
    }
    [[nodiscard]] double heuristic(const BlockPos& pos) const override {
        return static_cast<double>(std::min(std::abs(pos.x), std::abs(pos.z))) *
            action_costs::sprintOneBlock;
    }
    [[nodiscard]] std::string describe() const override { return "nearest axis"; }
};

// A finite form of Baritone's GoalRunAway. It is useful for retreating from a
// coordinate while still allowing A* to choose terrain-safe movement.
class GoalRunAway final : public Goal {
    BlockPos origin;
    int distance;
    bool maintainY;
public:
    GoalRunAway(BlockPos origin, int distance, bool maintainY = false)
        : origin(origin), distance(std::max(0, distance)), maintainY(maintainY) {}

    [[nodiscard]] bool isInGoal(const BlockPos& pos) const override {
        const auto dx = static_cast<long long>(pos.x) - origin.x;
        const auto dz = static_cast<long long>(pos.z) - origin.z;
        return dx * dx + dz * dz >= static_cast<long long>(distance) * distance &&
            (!maintainY || pos.y == origin.y);
    }
    [[nodiscard]] double heuristic(const BlockPos& pos) const override {
        const double dx = static_cast<double>(pos.x - origin.x);
        const double dz = static_cast<double>(pos.z - origin.z);
        const double remaining = std::max(0.0, static_cast<double>(distance) -
            std::sqrt(dx * dx + dz * dz));
        const double vertical = maintainY ? std::abs(pos.y - origin.y) *
            action_costs::jumpOneBlock() : 0.0;
        return remaining * action_costs::sprintOneBlock + vertical;
    }
    [[nodiscard]] std::string describe() const override {
        return "away from " + std::to_string(origin.x) + " " +
            std::to_string(origin.y) + " " + std::to_string(origin.z) +
            " by " + std::to_string(distance);
    }
};

} // namespace baritone
