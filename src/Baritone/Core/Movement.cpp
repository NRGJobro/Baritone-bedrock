#include "Movement.h"
#include "ActionCosts.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace baritone {
namespace {

bool passable(const BlockState& block, const PathOptions& options) {
    return block.loaded && !block.solid && !block.hazard && (!block.liquid || options.allowWater);
}

bool swimmable(const IWorld& world, const BlockPos& pos, const PathOptions& options) {
    if (!options.allowWater)
        return false;
    const auto feet = world.getBlock(pos);
    const auto head = world.getBlock(pos.offset(0, 1, 0));
    return feet.loaded && head.loaded && feet.liquid && !feet.hazard && !head.solid && !head.hazard;
}

bool safeWaterLanding(const IWorld& world, const BlockPos& pos) {
    const auto feet = world.getBlock(pos);
    const auto head = world.getBlock(pos.offset(0, 1, 0));
    return feet.loaded && head.loaded && feet.liquid && !feet.hazard &&
        !head.solid && !head.hazard;
}

} // namespace

bool MovementGenerator::canOccupy(const IWorld& world, const BlockPos& pos, const PathOptions& options) {
    return passable(world.getBlock(pos), options) && passable(world.getBlock(pos.offset(0, 1, 0)), options);
}

bool MovementGenerator::canStandAt(const IWorld& world, const BlockPos& pos, const PathOptions& options) {
    if (!canOccupy(world, pos, options))
        return false;

    const auto below = world.getBlock(pos.offset(0, -1, 0));
    return (below.loaded && below.solid && !below.hazard) || swimmable(world, pos, options);
}

std::vector<Movement> MovementGenerator::getMovements(const IWorld& world, const BlockPos& from, const PathOptions& options) {
    static constexpr std::array<std::pair<int, int>, 8> directions{{
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    }};

    std::vector<Movement> result;
    result.reserve(16);

    const bool fromWater = swimmable(world, from, options);
    const bool descendingWaterColumn = fromWater && swimmable(world, from.offset(0, -1, 0), options);

    for (const auto [dx, dz] : directions) {
        // When a continuous water column is directly below, stay in that
        // stream. Lateral water choices are deferred until the bottom, which
        // matches the fastest vanilla descent behavior.
        if (descendingWaterColumn && (dx != 0 || dz != 0))
            continue;
        const bool diagonal = dx != 0 && dz != 0;
        if (diagonal && !options.allowDiagonal)
            continue;

        if (diagonal) {
            if (!canOccupy(world, from.offset(dx, 0, 0), options) || !canOccupy(world, from.offset(0, 0, dz), options))
                continue;
        }

        const auto adjacent = from.offset(dx, 0, dz);
        const double horizontalCost = diagonal ? std::numbers::sqrt2 : 1.0;

        if (canStandAt(world, adjacent, options)) {
            const bool water = fromWater || swimmable(world, adjacent, options);
            const double baseCost = water ? action_costs::walkInWater :
                (options.preferSprint ? action_costs::sprintOneBlock : action_costs::walkOneBlock);
            result.push_back({adjacent, water ? MovementType::Swim : (diagonal ? MovementType::Diagonal : MovementType::Traverse), horizontalCost * baseCost});
            continue;
        }

        if (!diagonal && options.allowAscend) {
            const auto up = adjacent.offset(0, 1, 0);
            if (canStandAt(world, up, options) && canOccupy(world, from.offset(0, 1, 0), options)) {
                result.push_back({up, MovementType::Ascend, action_costs::jumpOneBlock()});
                continue;
            }
        }

        // Water cancels fall damage, so a clear loaded column ending in water
        // is always a valid fast descent. This deliberately ignores both the
        // ordinary Water toggle and maxFallHeight; the toggle controls route
        // swimming, not safe water landings.
        if (!diagonal) {
            constexpr int maximumLoadedDrop = 384;
            for (int drop = 1; drop <= maximumLoadedDrop; ++drop) {
                const auto cell = adjacent.offset(0, -drop, 0);
                const auto feetBlock = world.getBlock(cell);
                const auto headBlock = world.getBlock(cell.offset(0, 1, 0));
                if (!feetBlock.loaded || !headBlock.loaded || feetBlock.solid || headBlock.solid ||
                    feetBlock.hazard || headBlock.hazard)
                    break;
                if (safeWaterLanding(world, cell)) {
                    result.push_back({cell, MovementType::WaterDrop,
                        action_costs::walkOffBlock + action_costs::fallTicks(drop) * 0.72 +
                            action_costs::centerAfterFall});
                    break;
                }
            }
        }

        if (options.allowFall) {
            const auto down = adjacent.offset(0, -1, 0);
            if (canStandAt(world, down, options)) {
                result.push_back({down, MovementType::Descend,
                    horizontalCost * action_costs::walkOffBlock + action_costs::fallTicks(1) + action_costs::centerAfterFall});
                continue;
            }
        }

        if (!options.allowFall || diagonal || !canOccupy(world, adjacent, options))
            continue;

        for (int drop = 2; drop <= options.maxFallHeight; ++drop) {
            const auto landing = adjacent.offset(0, -drop, 0);
            if (!canOccupy(world, landing.offset(0, 1, 0), options))
                break;
            if (canStandAt(world, landing, options)) {
                result.push_back({landing, MovementType::Fall,
                    action_costs::walkOffBlock + action_costs::fallTicks(drop) + action_costs::centerAfterFall});
                break;
            }
        }
    }

    if (fromWater) {
        const auto up = from.offset(0, 1, 0);
        const auto down = from.offset(0, -1, 0);
        // Prefer an outward exit at the bottom of a water descent when one is
        // available. This prevents a visually misleading straight vertical
        // drop followed by an abrupt turn on the next node.
        static constexpr std::array<std::pair<int, int>, 4> exits{{
            {1, 0}, {-1, 0}, {0, 1}, {0, -1}
        }};
        for (const auto [dx, dz] : exits) {
            const auto exit = down.offset(dx, 0, dz);
            if (canStandAt(world, exit, options))
                result.push_back({exit, MovementType::Swim, action_costs::walkInWater + action_costs::centerAfterFall});
        }
        if (swimmable(world, up, options))
            result.push_back({up, MovementType::Swim, action_costs::walkInWater * 1.5});
        if (swimmable(world, down, options))
            // Descending with the stream is the preferred water movement: it
            // is faster than fighting the current laterally and keeps the
            // route centered until an exit exists at the bottom.
            result.push_back({down, MovementType::Swim, action_costs::walkInWater * 0.25});
    }

    // A mandatory water-drop landing still needs a way back onto land when
    // normal water routing is disabled. Permit only an immediate shoreline
    // exit; this does not enable general swimming behind the Water toggle.
    if (!options.allowWater && safeWaterLanding(world, from)) {
        static constexpr std::array<std::pair<int, int>, 4> shoreline{{
            {1, 0}, {-1, 0}, {0, 1}, {0, -1}
        }};
        for (const auto [dx, dz] : shoreline) {
            const auto exit = from.offset(dx, 0, dz);
            if (canStandAt(world, exit, options))
                result.push_back({exit, MovementType::Swim,
                    action_costs::walkInWater + action_costs::centerAfterFall});
        }
    }

    if (options.allowBridge && (!options.bridgeOnlyAfterFailure) && !fromWater && canStandAt(world, from, options)) {
        static constexpr std::array<std::pair<int, int>, 8> bridgeDirections{{
            {1, 0}, {-1, 0}, {0, 1}, {0, -1},
            {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
        }};
        // maxBridgeLength is the number of empty cells the player may fill.
        // The landing platform is therefore one block farther from the source.
        const int maxGapLength = std::clamp(options.maxBridgeLength, 1, 16);
        const int maxLandingDistance = maxGapLength + 1;
        for (const auto [dx, dz] : bridgeDirections) {
            const bool diagonalBridge = dx != 0 && dz != 0;
            if (diagonalBridge && !options.allowDiagonal)
                continue;
            const int firstDistance = diagonalBridge ? 1 : 2;
            const int lastDistance = diagonalBridge
                ? std::max(0, (maxGapLength + 1) / 2)
                : maxLandingDistance;
            for (int distance = firstDistance; distance <= lastDistance; ++distance) {
                const auto landing = from.offset(dx * distance, 0, dz * distance);
                if (!canStandAt(world, landing, options)) continue;
                bool clear = true;
                int requiredBlocks = distance - 1;
                if (diagonalBridge) {
                    // Build an edge-connected staircase: X, Z, X, Z... A row
                    // of corner-touching diagonal blocks is not a dependable
                    // vanilla walking surface.
                    requiredBlocks = distance * 2 - 1;
                    for (int step = 1; step <= distance && clear; ++step) {
                        const auto xStep = from.offset(dx * step, 0, dz * (step - 1));
                        if (!canOccupy(world, xStep, options) ||
                            !passable(world.getBlock(xStep.offset(0, 2, 0)), options)) {
                            clear = false;
                            break;
                        }
                        if (step < distance) {
                            const auto zStep = from.offset(dx * step, 0, dz * step);
                            if (!canOccupy(world, zStep, options) ||
                                !passable(world.getBlock(zStep.offset(0, 2, 0)), options))
                                clear = false;
                        }
                    }
                } else {
                    for (int step = 1; step < distance; ++step) {
                        const auto cell = from.offset(dx * step, 0, dz * step);
                        if (!canOccupy(world, cell, options) ||
                            !passable(world.getBlock(cell.offset(0, 2, 0)), options)) {
                            clear = false;
                            break;
                        }
                    }
                }
                if (clear)
                    result.push_back({landing, MovementType::Bridge,
                        action_costs::walkOneBlock * (diagonalBridge ? distance * std::numbers::sqrt2 : distance) +
                            static_cast<double>(requiredBlocks) * 9.0});
            }
        }
    }

    // Baritone-style cardinal parkour. Distances are source-to-destination:
    // 2 = one-block gap, 3 = two-block gap, 4 = three-block sprint gap.
    if (options.allowParkour && !fromWater && canStandAt(world, from, options) &&
        passable(world.getBlock(from.offset(0, 2, 0)), options)) {
        static constexpr std::array<std::pair<int, int>, 4> cardinal{{
            {1, 0}, {-1, 0}, {0, 1}, {0, -1}
        }};
        const int maxDistance = std::clamp(options.maxParkourDistance, 2, 4);

        for (const auto [dx, dz] : cardinal) {
            const auto adjacent = from.offset(dx, 0, dz);
            // If the adjacent block is walkable, normal traverse is safer.
            if (canStandAt(world, adjacent, options) || !canOccupy(world, adjacent, options) ||
                !passable(world.getBlock(adjacent.offset(0, 2, 0)), options))
                continue;

            for (int distance = 2; distance <= maxDistance; ++distance) {
                const auto candidate = from.offset(dx * distance, 0, dz * distance);
                bool clearArc = true;
                for (int step = 2; step < distance; ++step) {
                    const auto column = from.offset(dx * step, 0, dz * step);
                    if (!canOccupy(world, column, options) ||
                        !passable(world.getBlock(column.offset(0, 2, 0)), options)) {
                        clearArc = false;
                        break;
                    }
                }
                if (!clearArc)
                    break;

                if (canStandAt(world, candidate, options)) {
                    const auto overshoot = candidate.offset(dx, 0, dz);
                    const auto overFeet = world.getBlock(overshoot);
                    const auto overHead = world.getBlock(overshoot.offset(0, 1, 0));
                    // Match Java Baritone's overshoot safety: the first block
                    // after landing must not be hazardous, but it may be solid.
                    if (overFeet.loaded && overHead.loaded && !overFeet.hazard && !overHead.hazard)
                        result.push_back({candidate, MovementType::Parkour,
                            action_costs::parkourJump(distance, options.preferSprint)});
                    break;
                }

                if (options.allowParkourAscend && distance <= 3) {
                    const auto raised = candidate.offset(0, 1, 0);
                    if (canStandAt(world, raised, options) &&
                        passable(world.getBlock(candidate.offset(0, 3, 0)), options)) {
                        result.push_back({raised, MovementType::Parkour,
                            action_costs::parkourJump(distance, true) + action_costs::jumpOneBlock()});
                        break;
                    }
                }
            }
        }
    }

    return result;
}

} // namespace baritone
