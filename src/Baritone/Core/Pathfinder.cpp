#include "Pathfinder.h"

#include <algorithm>

namespace baritone {

void Pathfinder::begin(const BlockPos& startPos, std::shared_ptr<Goal> newGoal, const PathOptions newOptions) {
    options = newOptions;
    goal = std::move(newGoal);
    start = startPos;
    best = startPos;
    mostRecent = startPos;
    hasMostRecent = false;
    expanded = 0;
    result.clear();
    nodes.clear();
    open = {};

    if (!goal) {
        status = SearchStatus::Failed;
        return;
    }

    bestHeuristic = goal->heuristic(start);
    bestScore = bestHeuristic;
    auto& startNode = nodes[start];
    startNode.g = 0.0;
    startNode.f = bestHeuristic;
    open.push({start, startNode.f, startNode.g});
    status = SearchStatus::Searching;
}

SearchStatus Pathfinder::step(const IWorld& world, std::size_t budget) {
    if (status != SearchStatus::Searching)
        return status;

    if (budget == 0)
        budget = options.nodesPerTick;

    std::size_t processed = 0;
    while (!open.empty() && processed < budget) {
        const auto entry = open.top();
        open.pop();

        auto it = nodes.find(entry.pos);
        if (it == nodes.end())
            continue;

        auto& current = it->second;
        if (current.closed || entry.g != current.g)
            continue;

        current.closed = true;
        mostRecent = entry.pos;
        hasMostRecent = true;
        ++processed;
        ++expanded;

        const double h = goal->heuristic(entry.pos);
        // Java Baritone keeps several best-so-far candidates with weighted
        // cost coefficients. This equivalent Bedrock selection balances goal
        // progress against the actual Bedrock tick cost, preventing a cheap
        // heuristic-only branch from winning partial-path recovery.
        const double score = h + current.g / 2.0;
        if (score < bestScore) {
            bestScore = score;
            bestHeuristic = h;
            best = entry.pos;
        }

        if (goal->isInGoal(entry.pos)) {
            finish(SearchStatus::Found, entry.pos);
            return status;
        }

        if (expanded >= options.maxExpandedNodes) {
            finish(best == start ? SearchStatus::Failed : SearchStatus::Partial, best);
            return status;
        }

        for (const auto& movement : MovementGenerator::getMovements(world, entry.pos, options)) {
            auto& next = nodes[movement.destination];
            if (next.closed)
                continue;

            const double candidate = current.g + movement.cost;
            if (candidate >= next.g)
                continue;

            next.g = candidate;
            next.f = candidate + goal->heuristic(movement.destination);
            next.parent = entry.pos;
            next.hasParent = true;
            next.movement = movement.type;
            open.push({movement.destination, next.f, next.g});
        }
    }

    if (open.empty())
        finish(best == start ? SearchStatus::Failed : SearchStatus::Partial, best);

    return status;
}

void Pathfinder::cancel() {
    if (status == SearchStatus::Searching)
        status = SearchStatus::Cancelled;
    open = {};
}

SearchStatus Pathfinder::getStatus() const { return status; }

std::size_t Pathfinder::getExpandedNodeCount() const { return expanded; }

const std::vector<PathNode>& Pathfinder::getPath() const { return result; }

std::vector<PathNode> Pathfinder::getBestPathSoFar() const {
    return status == SearchStatus::Searching ? reconstruct(best) : std::vector<PathNode>{};
}

std::vector<PathNode> Pathfinder::getMostRecentPath() const {
    return status == SearchStatus::Searching && hasMostRecent ? reconstruct(mostRecent) : std::vector<PathNode>{};
}

void Pathfinder::finish(const SearchStatus newStatus, const BlockPos& end) {
    status = newStatus;
    result = reconstruct(end);
    open = {};
}

std::vector<PathNode> Pathfinder::reconstruct(const BlockPos& end) const {
    std::vector<PathNode> path;
    auto cursor = end;

    while (true) {
        const auto found = nodes.find(cursor);
        if (found == nodes.end())
            return {};

        path.push_back({cursor, found->second.movement});
        if (!found->second.hasParent)
            break;
        cursor = found->second.parent;
    }

    std::ranges::reverse(path);
    if (!path.empty())
        path.front().movement = MovementType::Start;
    return path;
}

} // namespace baritone
