#pragma once

#include "World.h"
#include "Pathfinder.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace baritone {

enum class RouteStage { Walk, Break, Build };

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

// Each search uses a copy: fallback must never raise the user's saved budget
// or leave construction/breaking enabled for the next navigation request.
inline PathOptions routeOptions(PathOptions options, RouteStage stage) {
    options.allowBreak = options.miningMode ? options.allowBreak : stage != RouteStage::Walk;
    options.allowBridge = options.allowBridge && stage == RouteStage::Build;
    options.bridgeOnlyAfterFailure = false;
    options.maxExpandedNodes = std::clamp<std::size_t>(options.maxExpandedNodes, 1, 4000);
    options.nodesPerTick = std::clamp<std::size_t>(options.nodesPerTick, 1, 96);
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
