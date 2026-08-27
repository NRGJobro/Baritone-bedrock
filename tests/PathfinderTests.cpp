#include "Baritone/Core/Goal.h"
#include "Baritone/Core/Pathfinder.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <unordered_set>

namespace {

class FakeWorld final : public baritone::IWorld {
public:
    int minLoadedX = -64;
    int maxLoadedX = 64;
    std::unordered_set<baritone::BlockPos, baritone::BlockPosHash> solid;
    std::unordered_set<baritone::BlockPos, baritone::BlockPosHash> hazard;

    FakeWorld() {
        for (int x = -64; x <= 64; ++x) {
            for (int z = -8; z <= 8; ++z)
                solid.insert({x, -1, z});
        }
    }

    [[nodiscard]] baritone::BlockState getBlock(const baritone::BlockPos& pos) const override {
        if (pos.x < minLoadedX || pos.x > maxLoadedX)
            return {.loaded = false};
        return {
            .loaded = true,
            .solid = solid.contains(pos),
            .liquid = false,
            .hazard = hazard.contains(pos)
        };
    }
};

baritone::SearchStatus run(baritone::Pathfinder& finder, const FakeWorld& world) {
    while (finder.getStatus() == baritone::SearchStatus::Searching)
        finder.step(world, 1000);
    return finder.getStatus();
}

void findsStraightPath() {
    FakeWorld world;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{8, 0, 0}));
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert((finder.getPath().front().pos == baritone::BlockPos{0, 0, 0}));
    assert((finder.getPath().back().pos == baritone::BlockPos{8, 0, 0}));
}

void climbsOneBlock() {
    FakeWorld world;
    world.solid.insert({2, 0, 0});
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{2, 1, 0}));
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert(std::ranges::any_of(finder.getPath(), [](const auto& node) { return node.movement == baritone::MovementType::Ascend; }));
}

void respectsStepUpToggle() {
    FakeWorld world;
    world.solid.insert({2, 0, 0});
    baritone::PathOptions options;
    options.allowAscend = false;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{2, 1, 0}), options);
    assert(run(finder, world) != baritone::SearchStatus::Found);
}

void respectsDropToggle() {
    FakeWorld world;
    world.solid.insert({0, 0, 0});
    baritone::PathOptions options;
    options.allowFall = false;
    options.allowAscend = false;
    baritone::Pathfinder finder;
    finder.begin({0, 1, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{1, 0, 0}), options);
    assert(run(finder, world) != baritone::SearchStatus::Found);
}

void avoidsHazards() {
    FakeWorld world;
    world.hazard.insert({2, 0, 0});
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{4, 0, 0}));
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert(std::ranges::none_of(finder.getPath(), [](const auto& node) { return node.pos == baritone::BlockPos{2, 0, 0}; }));
}

void returnsPartialAtUnloadedBoundary() {
    FakeWorld world;
    world.maxLoadedX = 3;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{10, 0, 0}));
    assert(run(finder, world) == baritone::SearchStatus::Partial);
    assert(!finder.getPath().empty());
    assert(finder.getPath().back().pos.x == 3);
}

void exposesCalculationPaths() {
    FakeWorld world;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{20, 0, 0}));
    assert(finder.step(world, 2) == baritone::SearchStatus::Searching);
    assert(finder.getBestPathSoFar().size() >= 2);
    assert(!finder.getMostRecentPath().empty());
}

} // namespace

int main() {
    findsStraightPath();
    climbsOneBlock();
    respectsStepUpToggle();
    respectsDropToggle();
    avoidsHazards();
    returnsPartialAtUnloadedBoundary();
    exposesCalculationPaths();
    std::cout << "Baritone core tests passed\n";
    return 0;
}
