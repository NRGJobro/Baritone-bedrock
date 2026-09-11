#include "Baritone/Core/Goal.h"
#include "Baritone/Core/AdvancedGoals.h"
#include "Baritone/Core/Movement.h"
#include "Baritone/Core/Pathfinder.h"
#include "Baritone/Core/NavigationPolicy.h"
#include "Baritone/Bedrock/BedrockPhysics.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <cmath>
#include <cstdlib>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
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

void continuationPreservesCommittedRoute() {
    using namespace baritone;
    assert(!shouldPlanContinuation(10, 100, 0));
    assert(shouldPlanContinuation(76, 100, 0));
    // A short appended segment does not cascade into hundreds of future nodes.
    assert(!shouldPlanContinuation(90, 105, 99));
    assert(shouldPlanContinuation(99, 105, 99));
    assert(shouldPlanContinuation(105, 105, 99));
    assert(!shouldPlanContinuation(0, 0, 0));
    std::vector<PathNode> path{
        {{0, 0, 0}, MovementType::Start, 0.0},
        {{1, 1, 0}, MovementType::Ascend, 7.0},
        {{2, 1, 0}, MovementType::Traverse, 3.0}
    };
    const auto original = path;
    // A search from the middle must not replace the current route.
    assert(!appendPathContinuation(path, {
        {{1, 1, 0}, MovementType::Start, 0.0},
        {{1, 1, 1}, MovementType::Traverse, 3.0}
    }));
    assert(!appendPathContinuation(path, {{{2, 1, 0}, MovementType::Start, 0.0}}));
    assert(path.size() == original.size());
    const std::size_t reachedEndpointIndex = path.size();
    assert(appendPathContinuation(path, {
        {{2, 1, 0}, MovementType::Start, 0.0},
        {{3, 1, 0}, MovementType::Traverse, 3.0},
        {{4, 0, 0}, MovementType::Descend, 4.0}
    }));
    for (std::size_t i = 0; i < original.size(); ++i) {
        assert(path[i].pos == original[i].pos);
        assert(path[i].movement == original[i].movement);
        assert(path[i].costFromPrevious == original[i].costFromPrevious);
    }
    // An executor already waiting at the endpoint resumes at the new edge.
    assert(path[reachedEndpointIndex].pos == (BlockPos{3, 1, 0}));
    assert(path[reachedEndpointIndex].movement == MovementType::Traverse);
}

void jumpLandingKeepsExistingRoute() {
    using namespace baritone;
    FakeWorld world;
    std::vector<PathNode> path{
        {{0, 0, 0}, MovementType::Start, 0.0},
        {{3, 0, 0}, MovementType::Parkour, 10.0},
        {{4, 0, 0}, MovementType::Traverse, 3.0},
        {{5, 0, 1}, MovementType::Diagonal, 4.0}
    };
    assert(jumpLandingIndex(path, 1, {3, 0, 0}, world) == 1);
    assert(jumpLandingIndex(path, 1, {4, 0, 0}, world) == 2);
    assert(jumpLandingIndex(path, 1, {5, 0, 1}, world) == 3);
    assert(jumpLandingIndex(path, 1, {4, 0, 1}, world) == path.size());
    world.solid.erase({4, -1, 0});
    assert(jumpLandingIndex(path, 1, {4, 0, 0}, world) == path.size());
    world.solid.insert({4, -1, 0});
    path[2].movement = MovementType::Parkour;
    assert(jumpLandingIndex(path, 1, {5, 0, 1}, world) == path.size());
}

void repairsAdjacentJumpLandingWithoutReplanning() {
    using namespace baritone;
    FakeWorld world;
    const std::vector<PathNode> original{
        {{0, 0, 0}, MovementType::Start, 0.0},
        {{3, 0, 0}, MovementType::Parkour, 10.0},
        {{4, 0, 0}, MovementType::Traverse, 3.0}
    };
    auto path = original;
    assert(repairJumpLanding(path, 1, {3, 0, 1}, world));
    assert(path.size() == original.size());
    assert(path[1].movement == MovementType::Traverse);
    assert(path[2].pos == original[2].pos);
    assert(path[2].costFromPrevious == original[2].costFromPrevious);
    path = original;
    world.solid.erase({3, -1, 1});
    assert(!repairJumpLanding(path, 1, {3, 0, 1}, world));
    world.solid.insert({3, -1, 1});
    world.solid.insert({3, 1, 0});
    assert(!repairJumpLanding(path, 1, {3, 0, 1}, world));
    assert(path[1].movement == MovementType::Parkour);
}

void continuationWaitsForKnownFooting() {
    using namespace baritone;
    FakeWorld world;
    world.solid.clear();
    world.solid.insert({0, 9, 0});
    world.solid.insert({1, 9, 0});
    world.maxLoadedX = 1;
    auto goal = std::make_shared<GoalBlock>(BlockPos{4, 10, 0});
    PathOptions options;
    options.allowBridge = false;
    options.allowBreak = false;
    options.allowParkour = false;
    Pathfinder finder;
    std::vector<PathNode> path{
        {{0, 10, 0}, MovementType::Start, 0.0},
        {{1, 10, 0}, MovementType::Traverse, 3.0}
    };
    finder.begin(path.back().pos, goal, options);
    assert(run(finder, world) == SearchStatus::Failed);
    assert(!appendPathContinuation(path, finder.getPath()));
    assert(path.size() == 2); // No invented movement off the platform.

    world.maxLoadedX = 4;
    for (int x = 2; x <= 4; ++x)
        world.solid.insert({x, 9, 0});
    finder.begin(path.back().pos, goal, options);
    assert(run(finder, world) == SearchStatus::Found);
    assert(appendPathContinuation(path, finder.getPath()));
    assert(path.size() == 5);
    assert(goal->isInGoal(path.back().pos));
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
    options.bridgeOnlyAfterFailure = false;
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

void waterDropsRespectMovementToggles() {
    FakeWorld world;
    world.solid.insert({0, 9, 0});
    world.liquid.insert({1, 0, 0});

    baritone::PathOptions options;
    options.allowParkour = false;
    options.allowBridge = false;
    baritone::Pathfinder finder;
    finder.begin({0, 10, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{2, 0, 0}), options);
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert(std::ranges::any_of(finder.getPath(), [](const auto& node) {
        return node.movement == baritone::MovementType::WaterDrop;
    }));
    for (int disabled = 0; disabled < 2; ++disabled) {
        options.allowWater = disabled != 0;
        options.allowFall = disabled == 0;
        const auto moves = baritone::MovementGenerator::getMovements(world, {0, 10, 0}, options);
        assert(std::ranges::none_of(moves, [](const auto& move) {
            return move.type == baritone::MovementType::WaterDrop;
        }));
    }
}

void crossesDeepLakeAtSurface() {
    FakeWorld world;
    for (int x = 1; x <= 12; ++x) {
        for (int z = -8; z <= 8; ++z) {
            world.solid.erase({x, -1, z});
            for (int y = -6; y <= 0; ++y)
                world.liquid.insert({x, y, z});
            world.solid.insert({x, -7, z});
        }
    }
    baritone::PathOptions options;
    options.allowParkour = false;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{13, 0, 0}), options);
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert(std::ranges::all_of(finder.getPath(), [](const auto& node) { return node.pos.y == 0; }));
    assert(std::ranges::any_of(finder.getPath(), [](const auto& node) {
        return node.movement == baritone::MovementType::Swim;
    }));
    options.maxExpandedNodes = 4;
    finder.begin({2, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{13, 0, 0}), options);
    assert(run(finder, world) == baritone::SearchStatus::Partial);
    assert(finder.getPath().size() > 1 && finder.getPath().back().pos.x > 2);
    assert(finder.getPath().back().pos.y == 0);
    options.maxExpandedNodes = 24000;
    // Starting on the lake floor must recover upward, even with solid footing.
    assert(!baritone::MovementGenerator::canStandAt(world, {6, -6, 0}, options));
    finder.begin({6, -6, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{13, 0, 0}), options);
    assert(run(finder, world) == baritone::SearchStatus::Found);
    for (std::size_t i = 1; i < finder.getPath().size(); ++i) {
        const auto& prev = finder.getPath()[i - 1].pos;
        const auto& next = finder.getPath()[i].pos;
        assert(next.y >= prev.y);
        if (prev.y < 0)
            assert(next.x == prev.x && next.z == prev.z && next.y == prev.y + 1);
    }
    // A raised bank has a real one-block climb out of the surface water.
    world.solid.insert({13, 0, 0});
    const auto moves = baritone::MovementGenerator::getMovements(world, {12, 0, 0}, options);
    assert(std::ranges::any_of(moves, [](const auto& move) {
        return move.destination == baritone::BlockPos{13, 1, 0} && move.type == baritone::MovementType::Ascend;
    }));
}

void rejectsUnsafeWaterAndUnknownMiningNeighbors() {
    FakeWorld world;
    baritone::PathOptions options;
    world.liquid.insert({1, 0, 0});
    world.hazard.insert({1, 0, 0});
    assert(!baritone::MovementGenerator::canStandAt(world, {1, 0, 0}, options));
    world.hazard.clear();
    world.solid.insert({1, 1, 0});
    assert(!baritone::MovementGenerator::canStandAt(world, {1, 0, 0}, options));
    world.maxLoadedX = 2;
    world.solid.insert({2, 0, 0});
    options.allowBreak = true;
    options.miningMode = true;
    const auto moves = baritone::MovementGenerator::getMovements(world, {1, 0, 1}, options);
    assert(baritone::MovementGenerator::wouldExposeLiquid(world, {2, 0, 0}));
    assert(std::ranges::none_of(moves, [](const auto& move) {
        return move.destination == baritone::BlockPos{2, 0, 0};
    }));
}

void respectsJumpClearanceAndAscentPolicy() {
    FakeWorld world;
    world.solid.erase({1, -1, 0});
    world.solid.insert({2, 2, 0});
    baritone::PathOptions options;
    options.allowDiagonal = false;
    auto moves = baritone::MovementGenerator::getMovements(world, {0, 0, 0}, options);
    assert(std::ranges::none_of(moves, [](const auto& move) {
        return move.type == baritone::MovementType::Parkour && move.destination == baritone::BlockPos{2, 0, 0};
    }));
    world.solid.erase({2, 2, 0});
    world.solid.insert({2, 0, 0});
    options.allowAscend = false;
    options.allowBridge = true;
    options.bridgeOnlyAfterFailure = false;
    moves = baritone::MovementGenerator::getMovements(world, {0, 0, 0}, options);
    assert(std::ranges::none_of(moves, [](const auto& move) {
        return move.destination.y > 0;
    }));
}

void pricesEachDescendingBreakOnce() {
    FakeWorld world;
    world.solid.insert({0, 0, 0});
    world.solid.insert({1, 0, 0});
    world.solid.insert({1, 1, 0});
    world.solid.insert({1, 2, 0});
    world.solid.insert({1, -1, 0});
    baritone::PathOptions options;
    options.allowBreak = true;
    options.miningMode = true;
    const auto moves = baritone::MovementGenerator::getMovements(world, {0, 1, 0}, options);
    const auto descent = std::ranges::find_if(moves, [](const auto& move) {
        return move.type == baritone::MovementType::BreakDescend && move.destination == baritone::BlockPos{1, 0, 0};
    });
    assert(descent != moves.end());
    assert(std::abs(descent->cost - (baritone::action_costs::walkOffBlock +
        baritone::action_costs::fallTicks(1) + 3.0)) < 1e-8);
}

void handlesGoalDistancesCorrectly() {
    const baritone::GoalBlock goal({0, 0, 0});
    assert(goal.heuristic({0, 10, 0}) < goal.heuristic({0, -10, 0}));
    const baritone::GoalNear near({0, 0, 0}, 3);
    assert(near.heuristic({2, 0, 0}) == 0.0);
    assert(!near.isInGoal({65536, 0, 0}));
    const baritone::GoalNear large({30000000, 0, 30000000}, 100000);
    assert(!large.isInGoal({-30000000, 0, -30000000}));
    assert(large.isInGoal({29950000, 0, 30000000}));
    const baritone::GoalGetToBlock interact({0, 0, 0});
    assert(interact.isInGoal({1, 0, 0}));
    assert(interact.heuristic({1, 0, 0}) == 0.0);
}

void zeroTickBudgetStillMakesProgress() {
    FakeWorld world;
    baritone::Pathfinder finder;
    baritone::PathOptions options;
    options.nodesPerTick = 0;
    finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{1, 0, 0}), options);
    finder.step(world);
    assert(finder.getExpandedNodeCount() == 1);
    assert(run(finder, world) == baritone::SearchStatus::Found);
}

void keepsWaterDropInsideLandingColumn() {
    using namespace baritone::bedrock_physics;
    for (float initial : {-0.18f, 0.18f}) {
        float offset = initial;
        float velocity = initial < 0.f ? 0.06f : -0.06f;
        for (int tick = 0; tick < 100; ++tick) {
            const float input = waterDropAxisInput(-offset, velocity, false);
            velocity = velocity * horizontalAirDrag + input * walkAirAcceleration;
            offset += velocity;
            assert(std::abs(offset) + playerHalfWidth < 0.5f);
        }
        assert(std::abs(offset) < 0.01f);
    }
    assert(waterDropAxisInput(-0.1f, 0.1f, false) < 0.f);
    assert(waterDropAxisInput(0.1f, -0.1f, false) > 0.f);
    FakeWorld world;
    world.liquid.insert({1, 0, 0});
    assert(baritone::MovementGenerator::canDropToWater(world, {0, 30, 0}, {1, 0, 0}));
    world.solid.insert({1, 14, 0}); // a leaf in the falling column
    assert(!baritone::MovementGenerator::canDropToWater(world, {0, 30, 0}, {1, 0, 0}));
    world.solid.erase({1, 14, 0});
    world.liquid.clear();
    assert(!baritone::MovementGenerator::canDropToWater(world, {0, 30, 0}, {1, 0, 0}));
}

void waterBobbingDoesNotCountAsProgress() {
    using baritone::bedrock_physics::madeWaterProgress;
    assert(!madeWaterProgress(0.01f, 0.5f, false));
    assert(!madeWaterProgress(0.01f, -0.5f, false));
    assert(madeWaterProgress(0.2f, 0.f, false));
    assert(madeWaterProgress(0.f, 0.5f, true));
}

void previewsContinueWithoutTerrainChanges() {
    using namespace baritone;
    const std::vector<PathNode> path{
        {{0, 0, 0}, MovementType::Start}, {{1, 0, 0}, MovementType::Traverse},
        {{2, 0, 0}, MovementType::Traverse}, {{3, 0, 0}, MovementType::BreakTraverse},
        {{4, 0, 0}, MovementType::Traverse}};
    const auto preview = pathSuffix(path, {0, 0, 0}, true);
    assert(preview.size() == 3);
    const auto continuation = pathSuffix(path, {1, 0, 0}, true);
    assert(continuation.size() == 2 && continuation.front().movement == MovementType::Start);
    assert(pathSuffix(path, {2, 0, 0}, true).size() == 1);
    assert(pathSuffix(path, {2, 0, 0}).size() == 3);
    assert(pathSuffix(path, {10, 0, 0}, true).empty());
    FakeWorld world;
    Pathfinder finder;
    finder.begin({0, 0, 0}, std::make_shared<GoalBlock>(BlockPos{40, 0, 0}));
    finder.step(world, 8);
    assert(finder.getStatus() == SearchStatus::Searching);
    const auto first = pathSuffix(finder.getBestPathSoFar(), {0, 0, 0}, true);
    assert(first.size() > 1);
    assert(run(finder, world) == SearchStatus::Found);
    assert(pathSuffix(finder.getPath(), first.back().pos).size() > 1);
}

void navigationUsesBoundedTerrainFallbacks() {
    baritone::PathOptions saved;
    saved.allowBridge = true;
    saved.maxExpandedNodes = 200000;
    saved.nodesPerTick = 10000;
    const auto walk = baritone::routeOptions(saved, baritone::RouteStage::Walk);
    const auto dig = baritone::routeOptions(saved, baritone::RouteStage::Break);
    const auto build = baritone::routeOptions(saved, baritone::RouteStage::Build);
    assert(!walk.allowBreak && !walk.allowBridge);
    assert(dig.allowBreak && !dig.allowBridge);
    assert(build.allowBreak && build.allowBridge);
    assert(build.maxExpandedNodes == 4000 && build.nodesPerTick == 96);
    assert(saved.maxExpandedNodes == 200000 && !saved.allowBreak);

    FakeWorld world;
    world.solid.insert({2, 0, 0});
    world.solid.insert({2, 1, 0});
    const auto goal = std::make_shared<baritone::GoalBlock>(baritone::BlockPos{2, 0, 0});
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0}, goal, walk);
    assert(run(finder, world) != baritone::SearchStatus::Found);
    finder.begin({0, 0, 0}, goal, dig);
    assert(run(finder, world) == baritone::SearchStatus::Found);
    assert(std::ranges::any_of(finder.getPath(), [](const auto& node) {
        return node.movement == baritone::MovementType::BreakTraverse;
    }));
}

void walksAroundInsteadOfBuildingStep() {
    FakeWorld world;
    world.solid.erase({1, -1, 0});
    world.solid.insert({2, 0, 0});
    baritone::PathOptions options;
    options.allowBridge = true;
    options.allowParkour = false;
    const auto moves = baritone::MovementGenerator::getMovements(world, {0, 0, 0}, options);
    assert(std::ranges::none_of(moves, [](const auto& move) {
        return move.type == baritone::MovementType::BuildAscend || move.type == baritone::MovementType::Bridge;
    }));
    for (auto stage : {baritone::RouteStage::Walk, baritone::RouteStage::Build}) {
        baritone::Pathfinder finder;
        finder.begin({0, 0, 0}, std::make_shared<baritone::GoalBlock>(baritone::BlockPos{2, 1, 0}),
            baritone::routeOptions(options, stage));
        assert(run(finder, world) == baritone::SearchStatus::Found);
        assert(std::ranges::none_of(finder.getPath(), [](const auto& node) {
            return node.movement == baritone::MovementType::BuildAscend || node.movement == baritone::MovementType::Bridge;
        }));
    }
}

void boundsRepeatedAndCyclicReplans() {
    baritone::ReplanGuard guard;
    for (int i = 0; i < 9; ++i)
        assert(guard.allow({0, 0, 0}, 100));
    assert(!guard.allow({0, 0, 0}, 100));
    guard = {};
    for (int i = 0; i < 10; ++i)
        assert(guard.allow({i % 2, 0, 0}, 100));
    assert(!guard.allow({0, 0, 0}, 100));
    guard = {};
    for (int i = 0; i < 100; ++i)
        assert(guard.allow({i, 0, 0}, 200 - i));
}

void entersTwoBlockHighLanding() {
    FakeWorld world;
    world.solid.insert({1, 0, 0});
    world.solid.insert({1, 3, 0}); // exactly two air cells above the step
    baritone::PathOptions options;
    const auto moves = baritone::MovementGenerator::getMovements(world, {0, 0, 0}, options);
    assert(std::ranges::any_of(moves, [](const auto& move) {
        return move.destination == baritone::BlockPos{1, 1, 0} && move.type == baritone::MovementType::Ascend;
    }));
    using baritone::bedrock_physics::shouldRetryAscent;
    assert(!shouldRetryAscent(false, 1.1f, 1.f, true, 8)); // head contact midair
    assert(!shouldRetryAscent(true, 1.f, 1.f, true, 8)); // supported on near edge
    assert(shouldRetryAscent(true, 0.f, 1.f, true, 8)); // actually fell back down
    assert(!baritone::MovementGenerator::isPlannedBreakCell({0, 0, 0}, {1, 1, 0},
        baritone::MovementType::Ascend, {1, 3, 0}));
    assert(baritone::MovementGenerator::isPlannedBreakCell({0, 0, 0}, {1, 1, 0},
        baritone::MovementType::BreakAscend, {0, 2, 0}));
    assert(!baritone::MovementGenerator::isPlannedBreakCell({0, 0, 0}, {1, 1, 0},
        baritone::MovementType::BreakAscend, {1, 0, 0}));
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
    assert(!parkourSprintReady(4, false, true, 10, 0.05f));
    assert(!parkourSprintReady(4, false, true, 0, 0.28f));
    assert(!parkourSprintReady(4, false, false, 10, 0.28f));
    assert(parkourSprintReady(4, false, true, 2, 0.24f));
    assert(!parkourSprintReady(3, false, true, 2, 0.10f));
    assert(parkourSprintReady(3, false, true, 2, 0.20f));
    assert(!parkourSprintReady(2, true, true, 2, 0.10f));
    assert(parkourSprintReady(2, false, false, 0, 0.10f));
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
#ifdef _MSC_VER
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    findsStraightPath();
    continuationPreservesCommittedRoute();
    jumpLandingKeepsExistingRoute();
    repairsAdjacentJumpLandingWithoutReplanning();
    continuationWaitsForKnownFooting();
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
    waterDropsRespectMovementToggles();
    crossesDeepLakeAtSurface();
    rejectsUnsafeWaterAndUnknownMiningNeighbors();
    respectsJumpClearanceAndAscentPolicy();
    pricesEachDescendingBreakOnce();
    handlesGoalDistancesCorrectly();
    zeroTickBudgetStillMakesProgress();
    keepsWaterDropInsideLandingColumn();
    waterBobbingDoesNotCountAsProgress();
    previewsContinueWithoutTerrainChanges();
    navigationUsesBoundedTerrainFallbacks();
    walksAroundInsteadOfBuildingStep();
    boundsRepeatedAndCyclicReplans();
    entersTwoBlockHighLanding();
    miningDisablesWaterDrops();
    miningRefusesBreaksThatWouldReleaseLiquid();
    miningCanDigAOneWideVerticalShaft();
    miningBuildsOnlyAcrossSafeWater();
    modelsBedrockPlayerPhysics();
    std::cout << "Limiter core tests passed\n";
    return 0;
}
