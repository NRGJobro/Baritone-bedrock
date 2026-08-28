#include "Baritone/Core/Goal.h"
#include "Baritone/Core/Movement.h"
#include "Baritone/Core/Pathfinder.h"
#include "Baritone/Bedrock/BedrockPhysics.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <cmath>
#include <unordered_set>

namespace {

class FakeWorld final : public baritone::IWorld {
public:
    int minLoadedX = -64;
    int maxLoadedX = 64;
    std::unordered_set<baritone::BlockPos, baritone::BlockPosHash> solid;
    std::unordered_set<baritone::BlockPos, baritone::BlockPosHash> hazard;
    std::unordered_set<baritone::BlockPos, baritone::BlockPosHash> liquid;

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
            .liquid = liquid.contains(pos),
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

void crossesOneBlockGapWithParkour() {
    FakeWorld world;
    for (int z = -8; z <= 8; ++z)
        world.solid.erase({2, -1, z});

    baritone::PathOptions options;
    options.allowDiagonal = false;
    options.allowFall = false;
    options.allowParkour = true;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{4, 0, 0}), options);
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert(std::ranges::any_of(finder.getPath(), [](const auto& node) {
        return node.movement == baritone::MovementType::Parkour;
    }));
}

void respectsParkourToggle() {
    FakeWorld world;
    for (int z = -8; z <= 8; ++z)
        world.solid.erase({2, -1, z});

    baritone::PathOptions options;
    options.allowDiagonal = false;
    options.allowFall = false;
    options.allowParkour = false;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{4, 0, 0}), options);
    assert(run(finder, world) != baritone::SearchStatus::Found);
}

void crossesThreeBlockGapWithSprintParkour() {
    FakeWorld world;
    for (int x = 2; x <= 4; ++x)
        for (int z = -8; z <= 8; ++z)
            world.solid.erase({x, -1, z});

    baritone::PathOptions options;
    options.allowDiagonal = false;
    options.allowFall = false;
    options.allowParkour = true;
    options.maxParkourDistance = 4;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{6, 0, 0}), options);
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert(std::ranges::any_of(finder.getPath(), [](const auto& node) {
        return node.movement == baritone::MovementType::Parkour;
    }));
}

void crossesFourBlockGapWithBridgeFallback() {
    FakeWorld world;
    for (int x = 1; x <= 4; ++x)
        for (int z = -8; z <= 8; ++z)
            world.solid.erase({x, -1, z});
    baritone::PathOptions options;
    options.allowDiagonal = false;
    options.allowFall = false;
    options.allowParkour = false;
    options.allowBridge = true;
    options.bridgeOnlyAfterFailure = false;
    options.maxBridgeLength = 8;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{5, 0, 0}), options);
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert(std::ranges::any_of(finder.getPath(), [](const auto& node) {
        return node.movement == baritone::MovementType::Bridge;
    }));
}

void crossesTenBlockGapAtConfiguredBridgeLimit() {
    FakeWorld world;
    for (int x = 1; x <= 10; ++x)
        for (int z = -12; z <= 12; ++z)
            world.solid.erase({x, -1, z});

    baritone::PathOptions options;
    options.allowDiagonal = false;
    options.allowFall = false;
    options.allowParkour = false;
    options.allowBridge = true;
    options.bridgeOnlyAfterFailure = false;
    options.maxBridgeLength = 10;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{11, 0, 0}), options);
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert(std::ranges::any_of(finder.getPath(), [](const auto& node) {
        return node.movement == baritone::MovementType::Bridge;
    }));
}

void discoversEdgeConnectedDiagonalBridge() {
    FakeWorld world;
    const std::array<baritone::BlockPos, 5> bridgeCells{{
        {1, -1, 0}, {1, -1, 1}, {2, -1, 1}, {2, -1, 2}, {3, -1, 2}
    }};
    for (const auto& cell : bridgeCells)
        world.solid.erase(cell);

    baritone::PathOptions options;
    options.allowDiagonal = true;
    options.allowBridge = true;
    options.bridgeOnlyAfterFailure = false;
    options.maxBridgeLength = 5;
    const auto movements = baritone::MovementGenerator::getMovements(world, {0, 0, 0}, options);
    assert(std::ranges::any_of(movements, [](const auto& movement) {
        return movement.type == baritone::MovementType::Bridge &&
            movement.destination == baritone::BlockPos{3, 0, 3};
    }));
}

void alwaysUsesSafeWaterDropRegardlessOfWaterToggle() {
    FakeWorld world;
    world.solid.insert({0, 9, 0});
    world.liquid.insert({1, 0, 0});

    baritone::PathOptions options;
    options.allowWater = false;
    options.allowFall = false;
    options.allowParkour = false;
    options.allowBridge = false;
    baritone::Pathfinder finder;
    finder.begin({0, 10, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{2, 0, 0}), options);
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert(std::ranges::any_of(finder.getPath(), [](const auto& node) {
        return node.movement == baritone::MovementType::WaterDrop;
    }));
}

void modelsBedrockPlayerPhysics() {
    using namespace baritone::bedrock_physics;
    float walkVelocity = 0.f;
    float sprintVelocity = 0.f;
    for (int tick = 0; tick < 100; ++tick) {
        walkVelocity = nextGroundVelocity(walkVelocity, 1.f, false, false);
        sprintVelocity = nextGroundVelocity(sprintVelocity, 1.f, true, false);
    }
    assert(std::abs(walkVelocity - 0.21585f) < 0.001f);
    assert(std::abs(sprintVelocity - 0.28060f) < 0.001f);
    assert(std::abs(safeTakeoffEdge - 0.775f) < 0.0001f);
    assert(projectedAirDisplacement(0.28f, 1.f, true, false, 5) >
        projectedAirDisplacement(0.28f, 0.f, true, false, 5));
    assert(ticksUntilLandingPlane(0.f, jumpVelocity, 0.f) > 1);
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
    crossesOneBlockGapWithParkour();
    respectsParkourToggle();
    crossesThreeBlockGapWithSprintParkour();
    crossesFourBlockGapWithBridgeFallback();
    crossesTenBlockGapAtConfiguredBridgeLimit();
    discoversEdgeConnectedDiagonalBridge();
    alwaysUsesSafeWaterDropRegardlessOfWaterToggle();
    modelsBedrockPlayerPhysics();
    std::cout << "Limiter core tests passed\n";
    return 0;
}
