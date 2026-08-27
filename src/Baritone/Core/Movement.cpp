#include "Movement.h"

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
            result.push_back({adjacent, water ? MovementType::Swim : (diagonal ? MovementType::Diagonal : MovementType::Traverse), horizontalCost * (water ? 2.2 : 1.0)});
            continue;
        }

        if (!diagonal && options.allowAscend) {
            const auto up = adjacent.offset(0, 1, 0);
            if (canStandAt(world, up, options) && canOccupy(world, from.offset(0, 1, 0), options)) {
                result.push_back({up, MovementType::Ascend, 1.55});
                continue;
            }
        }

        if (options.allowFall) {
            const auto down = adjacent.offset(0, -1, 0);
            if (canStandAt(world, down, options)) {
                result.push_back({down, MovementType::Descend, horizontalCost + 0.35});
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
                result.push_back({landing, MovementType::Fall, 1.0 + static_cast<double>(drop) * 0.65});
                break;
            }
        }
    }

    if (fromWater) {
        const auto up = from.offset(0, 1, 0);
        const auto down = from.offset(0, -1, 0);
        if (swimmable(world, up, options))
            result.push_back({up, MovementType::Swim, 2.4});
        if (swimmable(world, down, options))
            result.push_back({down, MovementType::Swim, 1.8});
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
                    if (passable(overFeet, options) && passable(overHead, options))
                        result.push_back({candidate, MovementType::Parkour,
                            static_cast<double>(distance) * (distance == 4 ? 0.82 : 1.0) + 0.75});
                    break;
                }

                if (options.allowParkourAscend && distance <= 3) {
                    const auto raised = candidate.offset(0, 1, 0);
                    if (canStandAt(world, raised, options) &&
                        passable(world.getBlock(candidate.offset(0, 3, 0)), options)) {
                        result.push_back({raised, MovementType::Parkour, static_cast<double>(distance) + 1.15});
                        break;
                    }
                }
            }
        }
    }

    return result;
}

} // namespace baritone
