#pragma once

#include "World.h"
#include "Pathfinder.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace baritone {

enum class RouteStage { Walk, Break, Build };

// Recognize a jump landing on the planned destination or its next two walking
// nodes. Exact block matches only; never skip another jump, drop or build.
inline std::size_t jumpLandingIndex(const std::vector<PathNode>& path,
    std::size_t index, const BlockPos& feet, const IWorld& world) {
    if (index >= path.size())
        return path.size();
    const auto movement = path[index].movement;
    if (movement != MovementType::Parkour && movement != MovementType::Ascend)
        return path.size();
    PathOptions landingOptions;
    landingOptions.allowWater = false;
    const auto end = std::min(path.size(), index + 3);
    for (std::size_t cursor = index; cursor < end; ++cursor) {
        if (cursor > index &&
            ((path[cursor].movement != MovementType::Traverse &&
                path[cursor].movement != MovementType::Diagonal) ||
                path[cursor].pos.y != path[index].pos.y))
            break;
        if (!MovementGenerator::canStandAt(world, path[cursor].pos, landingOptions))
            break;
        if (path[cursor].pos == feet)
            return cursor;
    }
    return path.size();
}

inline bool shouldPlanContinuation(std::size_t index, std::size_t pathSize,
    std::size_t nextPlanningIndex) {
    return pathSize > 0 && index >= nextPlanningIndex &&
        (index >= pathSize || pathSize - index <= 24);
}

// A supported landing beside the intended jump destination can finish with
// one ordinary walking edge. Validate that edge with the planner's collision
// rules instead of throwing away the rest of the route.
inline bool repairJumpLanding(std::vector<PathNode>& path, std::size_t index,
    const BlockPos& feet, const IWorld& world) {
    if (index == 0 || index >= path.size() ||
        (path[index].movement != MovementType::Parkour &&
            path[index].movement != MovementType::Ascend) ||
        feet == path[index - 1].pos || feet == path[index].pos ||
        feet.y != path[index].pos.y)
        return false;
    PathOptions options;
    options.allowWater = false;
    options.allowAscend = false;
    options.allowFall = false;
    options.allowParkour = false;
    if (!MovementGenerator::canStandAt(world, feet, options))
        return false;
    const auto moves = MovementGenerator::getMovements(world, feet, options);
    for (const auto& move : moves) {
        if (move.destination == path[index].pos &&
            (move.type == MovementType::Traverse || move.type == MovementType::Diagonal)) {
            path[index - 1].pos = feet;
            path[index].movement = move.type;
            path[index].costFromPrevious = move.cost;
            return true;
        }
    }
    return false;
}

// A separate search must join exactly at the committed endpoint. Preserve
// its incoming movement and cost; the new search's Start node is not an edge.
inline bool appendPathContinuation(std::vector<PathNode>& path,
    const std::vector<PathNode>& continuation) {
    if (path.empty() || continuation.size() < 2 ||
        path.back().pos != continuation.front().pos ||
        continuation.front().movement != MovementType::Start)
        return false;
    path.insert(path.end(), continuation.begin() + 1, continuation.end());
    return true;
}

inline std::vector<PathNode> pathSuffix(const std::vector<PathNode>& path, const BlockPos& position,
    bool naturalOnly = false) {
    auto begin = std::ranges::find(path, position, &PathNode::pos);
    if (begin == path.end())
        return {};
    auto end = path.end();
    if (naturalOnly) {
        end = std::find_if(begin + 1, end, [](const PathNode& node) {
            return node.movement == MovementType::Bridge || node.movement == MovementType::BuildAscend ||
                node.movement == MovementType::BreakTraverse || node.movement == MovementType::BreakAscend ||
                node.movement == MovementType::BreakDescend || node.movement == MovementType::BreakDown;
        });
    }
    std::vector<PathNode> result(begin, end);
    result.front().movement = MovementType::Start;
    result.front().costFromPrevious = 0.0;
    return result;
}

// During a straight-down break, a grounded actor is valid either immediately
// above the floor being mined or in the exact destination after that floor
// disappears. The executor observes node completion after its pre-mine guard,
// so the destination must not be mistaken for lateral path drift.
inline bool validBreakDownGroundCell(const BlockPos& source,
    const BlockPos& destination, const BlockPos& feet) {
    return feet == source || feet == destination;
}

// Centre once when entering a shaft. A completed BreakDown already lands in
// the validated shaft column, so requiring another centering pass between
// every block only creates visible left/right oscillation.
inline bool requiresBreakDownEntryCentering(const MovementType sourceMovement) {
    return sourceMovement != MovementType::BreakDown;
}

// Bedrock reports the feet point near cell boundaries with small collision
// and interpolation differences. A completed drop may therefore be outside
// the exact destination block even though it landed safely on the intended
// route strip. Keep this tolerance bounded to one landing width so unrelated
// lower terrain cannot consume the node.
inline bool validSupportedFallLanding(const bool supported, const float heightError,
    const float progress, const float lateralDistance) {
    return supported && std::isfinite(heightError) && std::isfinite(progress) &&
        std::isfinite(lateralDistance) && std::abs(heightError) <= 0.85f &&
        progress >= 0.30f && progress <= 2.25f && lateralDistance <= 0.72f;
}

inline int cameraIndependentParkourDistance(const int configuredDistance) {
    return std::clamp(configuredDistance, 2, 2);
}

// Each search uses a copy: fallback must never raise the user's saved budget
// or leave construction/breaking enabled for the next navigation request.
inline PathOptions routeOptions(PathOptions options, RouteStage stage) {
    options.allowBreak = options.miningMode ? options.allowBreak : stage != RouteStage::Walk;
    options.allowBridge = options.allowBridge && stage == RouteStage::Build;
    options.bridgeOnlyAfterFailure = false;
    const bool naturalWalk = stage == RouteStage::Walk && !options.miningMode;
    options.maxExpandedNodes = std::clamp<std::size_t>(options.maxExpandedNodes, 1,
        naturalWalk ? 24000 : 4000);
    options.nodesPerTick = std::clamp<std::size_t>(options.nodesPerTick, 1,
        naturalWalk ? 160 : 96);
    if (!options.miningMode)
        options.heuristicWeight = 1.0;
    return options;
}

// Includes failed movement, support conflicts and repeated partial endpoints.
// A move around a small loop must not reset the retry limit.
class ReplanGuard {
    std::unordered_map<BlockPos, int, BlockPosHash> visits;
    double bestHeuristic = std::numeric_limits<double>::infinity();
    int searchesWithoutProgress = 0;
public:
    bool allow(const BlockPos& start, double heuristic) {
        if (++visits[start] > 9)
            return false;
        if (heuristic < bestHeuristic - 0.5) {
            bestHeuristic = heuristic;
            searchesWithoutProgress = 0;
        } else if (++searchesWithoutProgress > 9) {
            return false;
        }
        return true;
    }
};

} // namespace baritone
