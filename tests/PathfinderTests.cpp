#include "Baritone/Core/Goal.h"
#include "Baritone/Core/AdvancedGoals.h"
#include "Baritone/Core/Movement.h"
#include "Baritone/Core/Pathfinder.h"
#include "Baritone/Core/NavigationPolicy.h"
#include "Baritone/Bedrock/BedrockPhysics.h"
#include "Baritone/Bedrock/MovementInput.h"
#include "Client/Modules/CameraTweaksMath.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <limits>
#include <cmath>
#include <cstdlib>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
#include <unordered_set>

namespace {

void cameraTweaksMatchPhaseBehavior() {
    using namespace CameraTweaksMath;
    assert(!acceptsScroll(0, true, false));
    assert(acceptsScroll(1, true, false));
    assert(acceptsScroll(2, true, false));
    assert(!acceptsScroll(1, false, false));
    assert(!acceptsScroll(1, true, true));
    assert(scrollDistance(4.f, 0.5f, true) == 3.5f);
    assert(scrollDistance(4.f, 0.5f, false) == 4.5f);
    assert(scrollDistance(0.5f, 4.f, true) == 0.5f);
    assert(scrollDistance(32.f, 4.f, false) == 32.f);
    assert(std::isfinite(scrollDistance(
        std::numeric_limits<float>::quiet_NaN(), 0.5f, true)));
    float current = 4.f;
    for (int i = 0; i < 120; ++i) {
        const float next = approach(current, 8.f, 1.f / 60.f);
        assert(next >= current && next <= 8.f);
        current = next;
    }
    assert(current > 7.99f);
    assert(approach(4.f, 8.f, 0.f) == 4.f);
}

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
    assert(walk.maxExpandedNodes == 24000 && walk.nodesPerTick == 160);
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
    assert(baritone::MovementGenerator::isPlannedBreakCell(
        {0, 0, 0}, {0, -1, 0}, baritone::MovementType::BreakDown, {0, 1, 0}));

    // Ordinary navigation may break obstructions, but it must not excavate a
    // walkable floor simply because the eventual goal is lower.
    options.miningMode = false;
    world.solid.insert({0, 1, 0});
    const auto ordinaryBreakMoves = baritone::MovementGenerator::getMovements(
        world, {0, 0, 0}, options);
    assert(std::ranges::none_of(ordinaryBreakMoves, [](const auto& movement) {
        return movement.destination == baritone::BlockPos{0, -1, 0} &&
            movement.type == baritone::MovementType::BreakDown;
    }));
}

void acceptsOnlySafeAlignedFallLandings() {
    using baritone::validSupportedFallLanding;
    assert(validSupportedFallLanding(true, 0.f, 0.30f, 0.72f));
    assert(validSupportedFallLanding(true, 0.85f, 2.25f, 0.f));
    assert(!validSupportedFallLanding(false, 0.f, 1.f, 0.f));
    assert(!validSupportedFallLanding(true, 0.86f, 1.f, 0.f));
    assert(!validSupportedFallLanding(true, 0.f, 0.29f, 0.f));
    assert(!validSupportedFallLanding(true, 0.f, 2.26f, 0.f));
    assert(!validSupportedFallLanding(true, 0.f, 1.f, 0.73f));
}

void ordinaryNavigationMinesDownOnlyFromIsolatedPillars() {
    FakeWorld world;
    baritone::PathOptions options;
    options.allowBreak = true;
    options.allowFall = true;
    options.miningMode = false;

    const auto surfaceMoves = baritone::MovementGenerator::getMovements(
        world, {0, 0, 0}, options);
    assert(std::ranges::none_of(surfaceMoves, [](const auto& movement) {
        return movement.type == baritone::MovementType::BreakDown;
    }));

    for (int y = 0; y <= 4; ++y)
        world.solid.insert({0, y, 0});
    const auto pillarMoves = baritone::MovementGenerator::getMovements(
        world, {0, 5, 0}, options);
    assert(std::ranges::any_of(pillarMoves, [](const auto& movement) {
        return movement.destination == baritone::BlockPos{0, 4, 0} &&
            movement.type == baritone::MovementType::BreakDown;
    }));
}

void walksThroughShallowDipInsteadOfParkour() {
    FakeWorld world;
    world.solid.erase({1, -1, 0});
    world.solid.insert({1, -2, 0});

    baritone::PathOptions options;
    options.allowDiagonal = false;
    options.allowParkour = true;
    options.maxParkourDistance = 2;
    const auto movements = baritone::MovementGenerator::getMovements(
        world, {0, 0, 0}, options);
    assert(std::ranges::none_of(movements, [](const auto& movement) {
        return movement.type == baritone::MovementType::Parkour;
    }));
    assert(std::ranges::any_of(movements, [](const auto& movement) {
        return movement.destination == baritone::BlockPos{1, -1, 0} &&
            movement.type == baritone::MovementType::Descend;
    }));
}

void keepsRuntimeParkourCameraIndependent() {
    // The live controller caps parkour at the one-gap form because it preserves
    // the player's real camera. Longer variants require aligned forward sprint
    // and are deliberately routed around at runtime.
    assert(baritone::cameraIndependentParkourDistance(4) == 2);
    assert(baritone::cameraIndependentParkourDistance(3) == 2);
    assert(baritone::cameraIndependentParkourDistance(2) == 2);
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

    // Shaft centering may release input to coast, but must never command a
    // reversal that can send the player toward the opposite edge.
    assert(shaftCenterInput(0.50f, 0.f) > 0.f);
    assert(shaftCenterInput(0.50f, -0.20f) > 0.f);
    assert(shaftCenterInput(0.50f, 0.20f) == 0.f);
    assert(shaftCenterInput(shaftCenteredAxisTolerance, 0.f) == 0.f);
    assert(shaftFootprintCentered(0.10f, -0.17f));
    assert(shaftFootprintCentered(shaftCenteredAxisTolerance,
        shaftCenteredAxisTolerance));
    assert(!shaftFootprintCentered(0.19f, 0.f));
    assert(!shaftFootprintCentered(0.f, -0.19f));
    for (int distanceStep = 0; distanceStep <= 100; ++distanceStep) {
        for (int speedStep = -100; speedStep <= 100; ++speedStep) {
            const float input = shaftCenterInput(
                static_cast<float>(distanceStep) / 100.f,
                static_cast<float>(speedStep) / 500.f);
            assert(std::isfinite(input));
            assert(input >= 0.f && input <= 0.32f);
        }
    }

    // A moving descent must choose the same path-axis correction regardless
    // of camera yaw. Exercise one- through four-block drops at both walking
    // and sprint-entry speeds and require a supported-block landing corridor.
    for (const float incomingSpeed : {0.21585f, 0.28060f}) {
        for (int height = 1; height <= 4; ++height) {
            const bool cautiousDrop = height >= 2;
            const float entryTarget = cautiousDrop ? 0.06f : 0.10f;
            const float landingTarget = cautiousDrop ? 0.80f : 0.90f;
            float y = static_cast<float>(height);
            float verticalVelocity = 0.f;
            float progress = 0.30f;
            // The last supported tick regulates a running approach before the
            // edge. This must tame even sprint momentum using ordinary input,
            // without directly changing velocity.
            const float approachInput = groundInputForTargetVelocity(incomingSpeed, entryTarget);
            float alongVelocity = nextGroundVelocity(
                incomingSpeed, approachInput, false, false);
            assert(std::abs(alongVelocity - entryTarget) < 0.0001f);
            for (int tick = 0; tick < 40 && y > 0.05f; ++tick) {
                const int remainingTicks = ticksUntilHeight(y, verticalVelocity, 0.f, 40);
                const float input = fallLandingInput(
                    progress, alongVelocity, 1.f, remainingTicks,
                    landingTarget);
                assert(input >= 0.f); // never produce a visible reverse tap
                alongVelocity = alongVelocity * horizontalAirDrag +
                    input * walkAirAcceleration;
                progress += alongVelocity;
                verticalVelocity = (verticalVelocity - gravity) * verticalDrag;
                y += verticalVelocity;
            }
            assert(progress >= (cautiousDrop ? 0.68f : 0.70f));
            assert(progress <= (cautiousDrop ? 0.92f : 1.10f));
        }
    }
    assert(!needsCautiousDropApproach(1, false, 0.8f, 0.28f, 1.f, 12));
    assert(!needsCautiousDropApproach(4, true, 0.8f, 0.28f, 1.f, 12));
    assert(needsCautiousDropApproach(2, false, 0.8f, 0.28f, 1.f, 8));

    // Multi-block drops use digital forward pulses on the supported block,
    // then release all air input. Both walk and sprint approaches must reach
    // the edge at a controlled speed without ever requesting reverse input.
    for (const float incomingSpeed : {0.21585f, 0.28060f}) {
        float speed = incomingSpeed;
        float progress = 0.f;
        for (int tick = 0; tick < 30 && progress < 0.80f; ++tick) {
            const bool sneaking = progress < cautiousDropSneakReleaseProgress;
            const float input = cautiousDropGroundInput(progress, speed);
            assert(input == 0.f || input == 1.f);
            speed = nextGroundVelocity(speed, input, false, sneaking);
            progress += speed;
        }
        assert(progress >= 0.80f);
        assert(speed <= 0.045f);
        for (int height = 2; height <= 4; ++height)
            assert(dropAirInput(height, progress, speed, 1.f, 12, 0.80f) == 0.f);
    }

    // Even an unrecoverably fast airborne entry now coasts rather than
    // producing a robotic one-tick backward input.
    assert(fallLandingInput(0.75f, 0.30f, 1.f, 8) == 0.f);
}

void miningDescendsBeforeCrossingExposedTerrain() {
    FakeWorld world;
    // A solid mass below an exposed starting platform gives the planner both
    // choices: walk toward X first, or open a safe vertical shaft and tunnel
    // after reaching the target depth.
    for (int y = -6; y <= -1; ++y) {
        for (int x = -1; x <= 7; ++x) {
            for (int z = -1; z <= 1; ++z)
                world.solid.insert({x, y, z});
        }
    }

    baritone::PathOptions options;
    options.allowBreak = true;
    options.allowFall = true;
    options.allowWater = false;
    options.allowParkour = false;
    options.allowBridge = false;
    options.miningMode = false;
    options.preferVerticalMining = true;
    options.preferredVerticalMiningY = -4;
    options.heuristicWeight = 1.75;
    baritone::Pathfinder finder;
    finder.begin({0, 0, 0},
        std::make_shared<baritone::GoalGetToBlock>(baritone::BlockPos{6, -4, 0}), options);
    assert(run(finder, world) == baritone::SearchStatus::Found);
    const auto& path = finder.getPath();
    assert(path.size() > 4);
    for (std::size_t index = 1; index <= 4; ++index) {
        assert(path[index].movement == baritone::MovementType::BreakDown);
        assert(path[index].pos.x == 0 && path[index].pos.z == 0);
    }

    const baritone::BlockPos source{0, 0, 0};
    const baritone::BlockPos destination{0, -1, 0};
    assert(baritone::validBreakDownGroundCell(source, destination, source));
    assert(baritone::validBreakDownGroundCell(source, destination, destination));
    assert(!baritone::validBreakDownGroundCell(source, destination, {1, -1, 0}));
    assert(baritone::requiresBreakDownEntryCentering(baritone::MovementType::Traverse));
    assert(!baritone::requiresBreakDownEntryCentering(baritone::MovementType::BreakDown));
}

void emitsVanillaSafeMovementInput() {
    using namespace baritone::movement_input;

    // Stress every quadrant, over-range diagonals, fractional steering, and
    // the exact zero crossings used as path correction settles.
    for (int x = -200; x <= 200; ++x) {
        for (int y = -200; y <= 200; ++y) {
            const auto movement = sanitize({x / 100.f, y / 100.f});
            assert(std::isfinite(movement.x));
            assert(std::isfinite(movement.y));
            assert(std::sqrt(movement.x * movement.x + movement.y * movement.y) <= 1.00001f);

            const auto state = directions(movement);
            assert(state.up == (movement.y > 0.f));
            assert(state.down == (movement.y < 0.f));
            assert(state.left == (movement.x < 0.f));
            assert(state.right == (movement.x > 0.f));
            assert(!(state.up && state.down));
            assert(!(state.left && state.right));
        }
    }

    assert(sanitize({0.019f, -0.019f}) == Vector{});
    assert(sanitize({1.f, 1.f}).x > 0.70f);
    assert(sanitize({1.f, 1.f}).y > 0.70f);
    assert(sanitize({std::numeric_limits<float>::infinity(), 1.f}) == Vector{});
    assert(sanitize({std::numeric_limits<float>::quiet_NaN(), 1.f}) == Vector{});
}

} // namespace

int main() {
#ifdef _MSC_VER
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    cameraTweaksMatchPhaseBehavior();
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
    walksThroughShallowDipInsteadOfParkour();
    respectsParkourToggle();
    crossesThreeBlockGapWithSprintParkour();
    keepsRuntimeParkourCameraIndependent();
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
    acceptsOnlySafeAlignedFallLandings();
    ordinaryNavigationMinesDownOnlyFromIsolatedPillars();
    miningDescendsBeforeCrossingExposedTerrain();
    miningBuildsOnlyAcrossSafeWater();
    modelsBedrockPlayerPhysics();
    emitsVanillaSafeMovementInput();
    std::cout << "Limiter core tests passed\n";
    return 0;
}
