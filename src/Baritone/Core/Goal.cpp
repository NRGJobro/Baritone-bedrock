#include "Goal.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <sstream>

namespace baritone {
namespace {

double octile(const BlockPos& from, const BlockPos& to) {
    const auto dx = static_cast<double>(std::abs(from.x - to.x));
    const auto dz = static_cast<double>(std::abs(from.z - to.z));
    const auto dy = static_cast<double>(std::abs(from.y - to.y));
    const auto diagonal = std::min(dx, dz);
    const auto straight = std::max(dx, dz) - diagonal;
    return diagonal * std::numbers::sqrt2 + straight + dy * 1.25;
}

std::string coordinates(const BlockPos& pos) {
    return std::to_string(pos.x) + " " + std::to_string(pos.y) + " " + std::to_string(pos.z);
}

} // namespace

GoalBlock::GoalBlock(const BlockPos target) : target(target) {}

bool GoalBlock::isInGoal(const BlockPos& pos) const { return pos == target; }

double GoalBlock::heuristic(const BlockPos& pos) const { return octile(pos, target); }

std::string GoalBlock::describe() const { return "block " + coordinates(target); }

const BlockPos& GoalBlock::getTarget() const { return target; }

GoalXZ::GoalXZ(const int x, const int z) : x(x), z(z) {}

bool GoalXZ::isInGoal(const BlockPos& pos) const { return pos.x == x && pos.z == z; }

double GoalXZ::heuristic(const BlockPos& pos) const { return octile(pos, {x, pos.y, z}); }

std::string GoalXZ::describe() const { return "xz " + std::to_string(x) + " " + std::to_string(z); }

int GoalXZ::getX() const { return x; }

int GoalXZ::getZ() const { return z; }

GoalYLevel::GoalYLevel(const int y) : y(y) {}

bool GoalYLevel::isInGoal(const BlockPos& pos) const { return pos.y == y; }

double GoalYLevel::heuristic(const BlockPos& pos) const { return std::abs(pos.y - y) * 1.25; }

std::string GoalYLevel::describe() const { return "y " + std::to_string(y); }

int GoalYLevel::getY() const { return y; }

GoalNear::GoalNear(const BlockPos target, const int radius) : target(target), radius(std::max(0, radius)) {}

bool GoalNear::isInGoal(const BlockPos& pos) const {
    const auto dx = pos.x - target.x;
    const auto dy = pos.y - target.y;
    const auto dz = pos.z - target.z;
    return dx * dx + dy * dy + dz * dz <= radius * radius;
}

double GoalNear::heuristic(const BlockPos& pos) const {
    return std::max(0.0, octile(pos, target) - static_cast<double>(radius));
}

std::string GoalNear::describe() const {
    return "near " + coordinates(target) + " radius " + std::to_string(radius);
}

const BlockPos& GoalNear::getTarget() const { return target; }

int GoalNear::getRadius() const { return radius; }

GoalComposite::GoalComposite(std::vector<std::shared_ptr<Goal>> goals) : goals(std::move(goals)) {}

bool GoalComposite::isInGoal(const BlockPos& pos) const {
    return std::ranges::any_of(goals, [&](const auto& goal) { return goal && goal->isInGoal(pos); });
}

double GoalComposite::heuristic(const BlockPos& pos) const {
    double best = std::numeric_limits<double>::infinity();
    for (const auto& goal : goals) {
        if (goal)
            best = std::min(best, goal->heuristic(pos));
    }
    return best;
}

std::string GoalComposite::describe() const { return "one of " + std::to_string(goals.size()) + " goals"; }

} // namespace baritone
