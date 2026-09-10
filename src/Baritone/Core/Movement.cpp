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
    return passable(feet, options) && passable(head, options) && feet.liquid && !head.liquid;
}

bool safeWaterLanding(const IWorld& world, const BlockPos& pos) {
    const auto feet = world.getBlock(pos);
    const auto head = world.getBlock(pos.offset(0, 1, 0));
    return feet.loaded && head.loaded && feet.liquid && !feet.hazard &&
        !feet.solid && !head.solid && !head.hazard && !head.liquid;
}

double breakCostForBlock(const IWorld& world, const BlockPos& pos, const PathOptions& options) {
    const auto block = world.getBlock(pos);
    if (!block.loaded || block.hazard || (block.liquid && !options.allowWater))
        return action_costs::costInf;
    if (!block.solid)
        return 0.0;
    if (!options.allowBreak || !block.breakable)
        return action_costs::costInf;
    if (MovementGenerator::wouldExposeLiquid(world, pos))
        return action_costs::costInf;
    // Mining is deliberately preferred for short underground corridors. The
    // executor still waits for Bedrock's real destroy progress; this planning
    // cost only prevents A* from choosing a huge detour around a wall when a
    // nearby break-and-walk route is available.
    return options.miningMode ? 1.0 : 24.0;
}

double breakCostToOccupy(const IWorld& world, const BlockPos& pos, const PathOptions& options) {
    const double feet = breakCostForBlock(world, pos, options);
    const double head = breakCostForBlock(world, pos.offset(0, 1, 0), options);
    if (feet >= action_costs::costInf || head >= action_costs::costInf)
        return action_costs::costInf;
    return feet + head;
}

bool hasSafeSupport(const IWorld& world, const BlockPos& pos) {
    const auto support = world.getBlock(pos.offset(0, -1, 0));
    return support.loaded && support.solid && !support.hazard;
}

bool safeBridgeWater(const IWorld& world, const BlockPos& supportPos) {
    const auto support = world.getBlock(supportPos);
    return support.loaded && support.liquid && !support.solid && !support.hazard;
}

} // namespace

bool MovementGenerator::canDropToWater(const IWorld& world, const BlockPos& from, const BlockPos& to) {
    if (to.y >= from.y || from.y - to.y > 384 ||
        std::abs(to.x - from.x) + std::abs(to.z - from.z) != 1 || !safeWaterLanding(world, to))
        return false;
    for (int y = to.y + 1; y <= from.y + 1; ++y) {
        const auto state = world.getBlock({to.x, y, to.z});
        if (!state.loaded || state.solid || state.hazard || state.liquid)
            return false;
    }
    return true;
}

bool MovementGenerator::isPlannedBreakCell(const BlockPos& from, const BlockPos& to,
    const MovementType type, const BlockPos& cell) {
    if (type == MovementType::BreakDown)
        return cell == to;
    if (type == MovementType::BreakTraverse || type == MovementType::BreakAscend ||
        type == MovementType::BreakDescend) {
        if (cell == to || cell == to.offset(0, 1, 0))
            return true;
        if (type == MovementType::BreakAscend)
            return cell == from.offset(0, 2, 0);
        if (type == MovementType::BreakDescend)
            return cell == to.offset(0, 2, 0);
    }
    return false;
}

bool MovementGenerator::wouldExposeLiquid(const IWorld& world, const BlockPos& pos) {
    // Fluids flow down and sideways, never upward. Checking the block above
    // plus four horizontal neighbors models every immediate opening caused by
    // removing this cell without rejecting a safe block merely because water
    // is underneath it.
    static constexpr std::array<BlockPos, 5> flowSources{{
        {0, 1, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}
    }};
    return std::ranges::any_of(flowSources, [&](const BlockPos& offset) {
        const auto neighbor = world.getBlock(pos.offset(offset.x, offset.y, offset.z));
        return !neighbor.loaded || neighbor.liquid;
    });
}

bool MovementGenerator::canOccupy(const IWorld& world, const BlockPos& pos, const PathOptions& options) {
    return passable(world.getBlock(pos), options) && passable(world.getBlock(pos.offset(0, 1, 0)), options);
}

bool MovementGenerator::canStandAt(const IWorld& world, const BlockPos& pos, const PathOptions& options) {
    if (!canOccupy(world, pos, options))
        return false;
    if (world.getBlock(pos).liquid || world.getBlock(pos.offset(0, 1, 0)).liquid)
        return swimmable(world, pos, options);

    const auto below = world.getBlock(pos.offset(0, -1, 0));
    return (below.loaded && below.solid && !below.hazard) || swimmable(world, pos, options);
}

std::vector<Movement> MovementGenerator::getMovements(const IWorld& world, const BlockPos& from, const PathOptions& options) {
    std::vector<Movement> result;
    getMovements(world, from, options, result);
    return result;
}

void MovementGenerator::getMovements(const IWorld& world, const BlockPos& from,
    const PathOptions& options, std::vector<Movement>& result) {
    static constexpr std::array<std::pair<int, int>, 8> directions{{
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    }};

    result.clear();
    if (result.capacity() < 24)
        result.reserve(24);

    const bool fromWater = options.allowWater && world.getBlock(from).liquid;
    // A submerged start recovers toward breathable water before travelling.
    // Never generate dives as shortcuts beneath a lake or shoreline.
    if (fromWater && world.getBlock(from.offset(0, 1, 0)).liquid) {
        const auto up = from.offset(0, 1, 0);
        if (canOccupy(world, from, options) && canOccupy(world, up, options))
            result.push_back({up, MovementType::Swim, action_costs::walkInWater * 1.5});
        return;
    }

    for (const auto [dx, dz] : directions) {
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

        if (!diagonal && hasSafeSupport(world, adjacent)) {
            const double breakCost = breakCostToOccupy(world, adjacent, options);
            if (breakCost > 0.0 && breakCost < action_costs::costInf)
                result.push_back({adjacent, MovementType::BreakTraverse,
                    action_costs::walkOneBlock + breakCost});
        }

        if (!diagonal && options.allowAscend) {
            const auto up = adjacent.offset(0, 1, 0);
            if (canStandAt(world, up, options) && canOccupy(world, from.offset(0, 1, 0), options)) {
                result.push_back({up, MovementType::Ascend, action_costs::jumpOneBlock()});
                continue;
            }
            // A mined ascent sweeps three breakable cells: the space above the
            // source player's head, then the raised destination's feet/head
            // column. Requiring the source ceiling to already be air removed
            // this transition inside solid deepslate and forced huge detours
            // through previously opened caves instead of a compact staircase.
            const double sourceCeilingCost = breakCostForBlock(
                world, from.offset(0, 2, 0), options);
            const double destinationCost = breakCostToOccupy(world, up, options);
            const double breakCost = sourceCeilingCost >= action_costs::costInf ||
                destinationCost >= action_costs::costInf
                ? action_costs::costInf
                : sourceCeilingCost + destinationCost;
            const auto step = world.getBlock(adjacent);
            if (step.loaded && step.solid && !step.hazard &&
                breakCost > 0.0 && breakCost < action_costs::costInf)
                result.push_back({up, MovementType::BreakAscend,
                    action_costs::jumpOneBlock() + breakCost});

        }

        // Build a short supported step over a gap, then jump onto the placed
        // block. This gives mining a believable way up when normal one-block
        // stairs are unavailable without enabling long parkour jumps.
        if (!diagonal && options.allowAscend && options.allowBridge && !options.bridgeOnlyAfterFailure && !options.bridgeOverWaterOnly &&
            canStandAt(world, from, options) &&
            passable(world.getBlock(adjacent), options) &&
            !canStandAt(world, adjacent, options)) {
            const auto raised = adjacent.offset(0, 1, 0);
            if (canOccupy(world, raised, options))
                result.push_back({raised, MovementType::BuildAscend,
                    action_costs::jumpOneBlock() + 40.0});
        }


        if (!diagonal && options.allowFall) {
            const auto down = adjacent.offset(0, -1, 0);
            const double destinationCost = breakCostToOccupy(world, down, options);
            // Before gravity lowers the feet, a descending player first sweeps
            // horizontally into the block at current head height. The lower
            // destination column alone misses this clearly visible obstruction.
            // A descending player first crosses the adjacent block at the
            // current feet height, then its head-height sweep, and only then
            // reaches the lower landing column. Include all three blocks in
            // the cost so route selection and execution agree on the visible
            // front-to-back mining order.
            const double feetSweepCost = breakCostForBlock(world, adjacent, options);
            const double upperSweepCost = breakCostForBlock(
                world, adjacent.offset(0, 1, 0), options);
            const double breakCost = destinationCost >= action_costs::costInf ||
                feetSweepCost >= action_costs::costInf ||
                upperSweepCost >= action_costs::costInf
                ? action_costs::costInf
                : destinationCost + upperSweepCost;
            if (hasSafeSupport(world, down) && breakCost > 0.0 && breakCost < action_costs::costInf)
                result.push_back({down, MovementType::BreakDescend,
                    action_costs::walkOffBlock + action_costs::fallTicks(1) + breakCost});
        }

        // For ordinary navigation, water cancels fall damage, so a clear
        // loaded column ending in water is a valid fast descent. Mining mode
        // deliberately excludes this shortcut because entering water can
        // derail the active mining route.
        const auto descentEntryHead = world.getBlock(adjacent.offset(0, 1, 0));
        if (options.allowWater && options.allowFall && !fromWater && !options.miningMode && !diagonal && descentEntryHead.loaded && !descentEntryHead.solid &&
            !descentEntryHead.hazard) {
            constexpr int maximumLoadedDrop = 384;
            for (int drop = 1; drop <= maximumLoadedDrop; ++drop) {
                const auto cell = adjacent.offset(0, -drop, 0);
                const auto feetBlock = world.getBlock(cell);
                const auto headBlock = world.getBlock(cell.offset(0, 1, 0));
                if (!feetBlock.loaded || !headBlock.loaded || feetBlock.solid || headBlock.solid ||
                    feetBlock.hazard || headBlock.hazard)
                    break;
                if (safeWaterLanding(world, cell) && canDropToWater(world, from, cell)) {
                    result.push_back({cell, MovementType::WaterDrop,
                        action_costs::walkOffBlock + action_costs::fallTicks(drop) * 0.72 +
                            action_costs::centerAfterFall});
                    break;
                }
            }
        }

        if (options.allowFall) {
            const auto down = adjacent.offset(0, -1, 0);
            if (canStandAt(world, down, options) && canOccupy(world, adjacent, options)) {
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

    // A mining descent does not need a two-column staircase. Remove the
    // single floor block directly under the player and fall one block in the
    // same X/Z column, provided the next floor is loaded, solid, and safe.
    // Fluid exposure, hazards, and unbreakable blocks are rejected by the
    // same break/support checks used by every other mining transition.
    if (options.miningMode && options.allowFall && options.allowBreak && !fromWater) {
        const auto down = from.offset(0, -1, 0);
        const double breakCost = breakCostToOccupy(world, down, options);
        if (hasSafeSupport(world, down) && breakCost > 0.0 &&
            breakCost < action_costs::costInf) {
            result.push_back({down, MovementType::BreakDown,
                action_costs::fallTicks(1) + action_costs::centerAfterFall + breakCost});
        }
    }

    // Mining's construction policy is intentionally narrow and cheap to
    // search: from a shoreline, scan each cardinal direction until the first
    // solid landing and require every missing support cell to be safe water.
    if (options.allowBridge && !options.bridgeOnlyAfterFailure && options.bridgeOverWaterOnly &&
        !fromWater && canStandAt(world, from, options)) {
        static constexpr std::array<std::pair<int, int>, 4> cardinal{{
            {1, 0}, {-1, 0}, {0, 1}, {0, -1}
        }};
        const int maxGapLength = std::clamp(options.maxBridgeLength, 1, 16);
        for (const auto [dx, dz] : cardinal) {
            for (int distance = 2; distance <= maxGapLength + 1; ++distance) {
                const int gapStep = distance - 1;
                const auto support = from.offset(dx * gapStep, -1, dz * gapStep);
                const auto gapCell = from.offset(dx * gapStep, 0, dz * gapStep);
                if (!safeBridgeWater(world, support) ||
                    !canOccupy(world, gapCell, options) ||
                    !passable(world.getBlock(gapCell.offset(0, 2, 0)), options))
                    break;

                const auto landing = from.offset(dx * distance, 0, dz * distance);
                if (!canStandAt(world, landing, options))
                    continue;
                result.push_back({landing, MovementType::Bridge,
                    action_costs::walkOneBlock * distance +
                        static_cast<double>(gapStep) * 40.0});
                break;
            }
        }
    }

    if (options.allowBridge && (!options.bridgeOnlyAfterFailure) &&
        !options.bridgeOverWaterOnly && !fromWater && canStandAt(world, from, options)) {
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
                            static_cast<double>(requiredBlocks) * 40.0});
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

                if (canStandAt(world, candidate, options) &&
                    passable(world.getBlock(candidate.offset(0, 2, 0)), options)) {
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

                if (options.allowAscend && options.allowParkourAscend && distance <= 3) {
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

}

} // namespace baritone
