#include "Baritone/Core/Goal.h"
#include "Baritone/Core/AdvancedGoals.h"
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
    std::unordered_set<baritone::BlockPos, baritone::BlockPosHash> unbreakable;

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
            .hazard = hazard.contains(pos),
            .breakable = solid.contains(pos) && !unbreakable.contains(pos)
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

void buildsAndClimbsOverGap() {
    FakeWorld world;
    world.solid.erase({1, -1, 0});
    baritone::PathOptions options;
    options.allowBridge = true;
    options.allowParkour = false;
    const auto movements = baritone::MovementGenerator::getMovements(world, {0, 0, 0}, options);
    assert(std::ranges::any_of(movements, [](const auto& movement) {
        return movement.destination == baritone::BlockPos{1, 1, 0} &&
            movement.type == baritone::MovementType::BuildAscend;
    }));
}

void clearsUpperHeadCellBeforeDescending() {
    FakeWorld world;
    world.solid.insert({0, 0, 0});
    world.solid.insert({1, 2, 0});

    baritone::PathOptions options;
    options.allowDiagonal = false;
    options.allowParkour = false;
    options.allowBridge = false;
    auto movements = baritone::MovementGenerator::getMovements(world, {0, 1, 0}, options);
    assert(std::ranges::none_of(movements, [](const auto& movement) {
        return movement.destination == baritone::BlockPos{1, 0, 0};
    }));

    options.allowBreak = true;
    movements = baritone::MovementGenerator::getMovements(world, {0, 1, 0}, options);
    assert(std::ranges::any_of(movements, [](const auto& movement) {
        return movement.destination == baritone::BlockPos{1, 0, 0} &&
            movement.type == baritone::MovementType::BreakDescend;
    }));
}

void minesThroughWallWhenAllowed() {
    FakeWorld world;
    for (int y = 0; y <= 3; ++y) {
        for (int z = -8; z <= 8; ++z)
            world.solid.insert({2, y, z});
    }

    baritone::PathOptions options;
    options.allowBreak = true;
    options.allowBridge = false;
    options.allowParkour = false;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{4, 0, 0}), options);
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert(std::ranges::any_of(finder.getPath(), [](const auto& node) {
        return node.movement == baritone::MovementType::BreakTraverse ||
            node.movement == baritone::MovementType::BreakAscend ||
            node.movement == baritone::MovementType::BreakDescend;
    }));
}

void minesAnEnclosedStairStepUp() {
    FakeWorld world;
    // The adjacent lower block remains as the stair tread. The source sweep
    // and raised destination column are all solid and must be mined.
    world.solid.insert({1, 0, 0});
    world.solid.insert({0, 2, 0});
    world.solid.insert({1, 1, 0});
    world.solid.insert({1, 2, 0});

    baritone::PathOptions options;
    options.allowBreak = true;
    options.allowAscend = true;
    options.allowWater = false;
    options.allowParkour = false;
    options.allowBridge = false;
    options.miningMode = true;
    auto movements = baritone::MovementGenerator::getMovements(world, {0, 0, 0}, options);
    assert(std::ranges::any_of(movements, [](const auto& movement) {
        return movement.destination == baritone::BlockPos{1, 1, 0} &&
            movement.type == baritone::MovementType::BreakAscend;
    }));

    // The same shortcut must disappear if its overhead sweep is bedrock or
    // another unbreakable block.
    world.unbreakable.insert({0, 2, 0});
    movements = baritone::MovementGenerator::getMovements(world, {0, 0, 0}, options);
    assert(std::ranges::none_of(movements, [](const auto& movement) {
        return movement.destination == baritone::BlockPos{1, 1, 0} &&
            movement.type == baritone::MovementType::BreakAscend;
    }));
}

void findsACompactStaircaseThroughSolidRock() {
    FakeWorld world;
    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            for (int y = 0; y <= 6; ++y)
                world.solid.insert({x, y, z});
        }
    }
    world.solid.erase({0, 0, 0});
    world.solid.erase({0, 1, 0});

    baritone::PathOptions options;
    options.allowBreak = true;
    options.allowAscend = true;
    options.allowWater = false;
    options.allowDiagonal = false;
    options.allowParkour = false;
    options.allowBridge = false;
    options.miningMode = true;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(
        baritone::BlockPos{0, 4, 0}), options);
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert(finder.getPath().size() <= 8);
    assert(std::ranges::count_if(finder.getPath(), [](const auto& node) {
        return node.movement == baritone::MovementType::BreakAscend;
    }) >= 4);
}

void respectsBreakToggleAndUnbreakableBlocks() {
    FakeWorld world;
    for (int y = 0; y <= 3; ++y) {
        for (int z = -8; z <= 8; ++z)
            world.solid.insert({2, y, z});
    }

    baritone::PathOptions options;
    options.allowBreak = false;
    options.allowBridge = false;
    options.allowParkour = false;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{4, 0, 0}), options);
    assert(run(finder, world) != baritone::SearchStatus::Found);

    world.unbreakable = world.solid;
    options.allowBreak = true;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{4, 0, 0}), options);
    assert(run(finder, world) != baritone::SearchStatus::Found);
}

void requiresFullPlayerHeightEverywhere() {
    FakeWorld world;
    baritone::PathOptions options;

    // A solid destination head cell makes a one-block-high opening invalid
    // even though its feet cell is empty.
    world.solid.insert({1, 1, 0});
    auto movements = baritone::MovementGenerator::getMovements(world, {0, 0, 0}, options);
    assert(std::ranges::none_of(movements, [](const auto& movement) {
        return movement.destination == baritone::BlockPos{1, 0, 0};
    }));

    // A one-block ascent needs the extra cell swept by the player's head over
    // the source column during the jump arc.
    world.solid.erase({1, 1, 0});
    world.solid.insert({1, 0, 0});
    world.solid.insert({0, 2, 0});
    movements = baritone::MovementGenerator::getMovements(world, {0, 0, 0}, options);
    assert(std::ranges::none_of(movements, [](const auto& movement) {
        return movement.destination == baritone::BlockPos{1, 1, 0};
    }));
}

void exposesMovementCostsForEta() {
    FakeWorld world;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{8, 0, 0}));
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert(finder.getPathCost() > 0.0);
    assert(finder.getPath().front().costFromPrevious == 0.0);
    double reconstructedCost = 0.0;
    for (const auto& node : finder.getPath())
        reconstructedCost += node.costFromPrevious;
    assert(std::abs(reconstructedCost - finder.getPathCost()) < 0.0001);
}

void supportsAdvancedBaritoneGoals() {
    const baritone::GoalAxis axis;
    assert(axis.isInGoal({0, 20, 400}));
    assert(axis.isInGoal({-300, 5, 0}));
    assert(!axis.isInGoal({3, 5, 4}));
    assert(axis.heuristic({3, 5, 4}) > 0.0);

    const baritone::GoalRunAway away({0, 10, 0}, 8);
    assert(!away.isInGoal({7, 10, 0}));
    assert(away.isInGoal({8, 30, 0}));
    assert(away.heuristic({7, 10, 0}) > away.heuristic({8, 10, 0}));

    const baritone::GoalThreeBlocks vein({4, 12, 9});
    assert(vein.isInGoal({4, 12, 9}));
    assert(vein.isInGoal({4, 10, 9}));
    assert(!vein.isInGoal({4, 9, 9}));
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

void miningDisablesWaterDrops() {
    FakeWorld world;
    world.solid.insert({0, 9, 0});
    world.liquid.insert({1, 0, 0});

    baritone::PathOptions options;
    options.miningMode = true;
    options.allowWater = false;
    options.allowFall = false;
    options.allowParkour = false;
    options.allowBridge = false;
    const auto movements = baritone::MovementGenerator::getMovements(world, {0, 10, 0}, options);
    assert(std::ranges::none_of(movements, [](const auto& movement) {
        return movement.type == baritone::MovementType::WaterDrop;
    }));
}

void miningRefusesBreaksThatWouldReleaseLiquid() {
    FakeWorld world;
    world.solid.insert({1, 0, 0});
    world.liquid.insert({1, 0, 1});

    baritone::PathOptions options;
    options.allowBreak = true;
    options.allowWater = false;
    options.miningMode = true;
    const auto movements = baritone::MovementGenerator::getMovements(world, {0, 0, 0}, options);
    assert(std::ranges::none_of(movements, [](const auto& movement) {
        return movement.destination == baritone::BlockPos{1, 0, 0} &&
            movement.type == baritone::MovementType::BreakTraverse;
    }));
}

void miningCanDigAOneWideVerticalShaft() {
    FakeWorld world;
    world.solid.insert({0, -2, 0});

    baritone::PathOptions options;
    options.allowBreak = true;
    options.allowFall = true;
    options.allowWater = false;
    options.miningMode = true;
    const auto movements = baritone::MovementGenerator::getMovements(world, {0, 0, 0}, options);
    assert(std::ranges::any_of(movements, [](const auto& movement) {
        return movement.destination == baritone::BlockPos{0, -1, 0} &&
            movement.type == baritone::MovementType::BreakDown;
    }));
}

void miningBuildsOnlyAcrossSafeWater() {
    FakeWorld waterWorld;
    for (int x = 1; x <= 3; ++x) {
        waterWorld.solid.erase({x, -1, 0});
        waterWorld.liquid.insert({x, -1, 0});
    }

    baritone::PathOptions options;
    options.allowWater = false;
    options.allowBridge = true;
    options.bridgeOnlyAfterFailure = false;
    options.bridgeOverWaterOnly = true;
    options.allowParkour = false;
    options.miningMode = true;
    auto movements = baritone::MovementGenerator::getMovements(waterWorld, {0, 0, 0}, options);
    assert(std::ranges::any_of(movements, [](const auto& movement) {
        return movement.destination == baritone::BlockPos{4, 0, 0} &&
            movement.type == baritone::MovementType::Bridge;
    }));

    FakeWorld dryGapWorld;
    for (int x = 1; x <= 3; ++x)
        dryGapWorld.solid.erase({x, -1, 0});
    movements = baritone::MovementGenerator::getMovements(dryGapWorld, {0, 0, 0}, options);
    assert(std::ranges::none_of(movements, [](const auto& movement) {
        return movement.type == baritone::MovementType::Bridge ||
            movement.type == baritone::MovementType::BuildAscend;
    }));

    waterWorld.hazard.insert({2, -1, 0});
    movements = baritone::MovementGenerator::getMovements(waterWorld, {0, 0, 0}, options);
    assert(std::ranges::none_of(movements, [](const auto& movement) {
        return movement.type == baritone::MovementType::Bridge;
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
    buildsAndClimbsOverGap();
    respectsStepUpToggle();
    requiresFullPlayerHeightEverywhere();
    respectsDropToggle();
    clearsUpperHeadCellBeforeDescending();
    avoidsHazards();
    minesThroughWallWhenAllowed();
    minesAnEnclosedStairStepUp();
    findsACompactStaircaseThroughSolidRock();
    respectsBreakToggleAndUnbreakableBlocks();
    returnsPartialAtUnloadedBoundary();
    exposesCalculationPaths();
    exposesMovementCostsForEta();
    supportsAdvancedBaritoneGoals();
    crossesOneBlockGapWithParkour();
    respectsParkourToggle();
    crossesThreeBlockGapWithSprintParkour();
    crossesFourBlockGapWithBridgeFallback();
    crossesTenBlockGapAtConfiguredBridgeLimit();
    discoversEdgeConnectedDiagonalBridge();
    alwaysUsesSafeWaterDropRegardlessOfWaterToggle();
    miningDisablesWaterDrops();
    miningRefusesBreaksThatWouldReleaseLiquid();
    miningCanDigAOneWideVerticalShaft();
    miningBuildsOnlyAcrossSafeWater();
    modelsBedrockPlayerPhysics();
    std::cout << "Limiter core tests passed\n";
    return 0;
}
