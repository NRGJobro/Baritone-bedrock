#include "Pathfinder.h"

#include <algorithm>
#include <chrono>

namespace baritone {
namespace {

class CachedWorld final : public IWorld {
    const IWorld& source;
    std::unordered_map<BlockPos, BlockState, BlockPosHash>& cache;

public:
    CachedWorld(const IWorld& source,
        std::unordered_map<BlockPos, BlockState, BlockPosHash>& cache)
        : source(source), cache(cache) {}

    [[nodiscard]] BlockState getBlock(const BlockPos& pos) const override {
        if (const auto found = cache.find(pos); found != cache.end())
            return found->second;

        const auto state = source.getBlock(pos);
        // Do not retain unloaded cells. Chunks may finish loading while a
        // multi-tick search is in progress, and a stale unloaded result would
        // incorrectly turn a newly available route into a dead end.
        if (state.loaded)
            cache.emplace(pos, state);
        return state;
    }
};

} // namespace

void Pathfinder::begin(const BlockPos& startPos, std::shared_ptr<Goal> newGoal, const PathOptions newOptions) {
    options = newOptions;
    goal = std::move(newGoal);
    start = startPos;
    best = startPos;
    mostRecent = startPos;
    hasMostRecent = false;
    expanded = 0;
    result.clear();
    resultCost = 0.0;
    nodes.clear();
    worldCache.clear();
    const auto reserveNodes = std::min<std::size_t>(options.maxExpandedNodes, 65536);
    nodes.reserve(reserveNodes);
    worldCache.reserve(std::min<std::size_t>(options.maxExpandedNodes * 2, 131072));
    std::vector<QueueEntry> openStorage;
    openStorage.reserve(reserveNodes);
    open = decltype(open)(QueueCompare{}, std::move(openStorage));

    if (!goal) {
        status = SearchStatus::Failed;
        return;
    }

    bestHeuristic = goal->heuristic(start);
    bestScore = bestHeuristic;
    auto& startNode = nodes[start];
    startNode.g = 0.0;
    startNode.f = bestHeuristic * std::clamp(options.heuristicWeight, 1.0, 4.0);
    open.push({start, startNode.f, startNode.g});
    status = SearchStatus::Searching;
}

SearchStatus Pathfinder::step(const IWorld& world, std::size_t budget) {
    if (status != SearchStatus::Searching)
        return status;

    if (budget == 0)
        budget = std::max<std::size_t>(1, options.nodesPerTick);

    CachedWorld cachedWorld(world, worldCache);
    // Reuse one successor buffer for the complete tick slice. Previously each
    // expanded node allocated and freed a vector on the game's main thread.
    std::vector<Movement> movements;
    movements.reserve(24);

    std::size_t processed = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2);
    while (!open.empty() && processed < budget) {
        if (processed > 0 && std::chrono::steady_clock::now() >= deadline)
            break;
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
        // A partial path must retain actual goal progress even for expensive
        // swimming/mining moves. Adding cost to the heuristic as the primary
        // criterion can leave 'best' at the start of a perfectly usable lake
        // crossing. Use cost to choose between equally close frontier nodes.
        const double score = h + current.g / 2.0;
        if (h < bestHeuristic || (h == bestHeuristic && score < bestScore)) {
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

        MovementGenerator::getMovements(cachedWorld, entry.pos, options, movements);
        for (const auto& movement : movements) {
            auto& next = nodes[movement.destination];
            if (next.closed)
                continue;

            const double candidate = current.g + movement.cost;
            if (candidate >= next.g)
                continue;

            next.g = candidate;
            next.f = candidate + goal->heuristic(movement.destination) *
                std::clamp(options.heuristicWeight, 1.0, 4.0);
            next.parent = entry.pos;
            next.hasParent = true;
            next.movement = movement.type;
            next.movementCost = movement.cost;
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

double Pathfinder::getPathCost() const { return resultCost; }

std::vector<PathNode> Pathfinder::getBestPathSoFar() const {
    return status == SearchStatus::Searching ? reconstruct(best) : std::vector<PathNode>{};
}

std::vector<PathNode> Pathfinder::getMostRecentPath() const {
    return status == SearchStatus::Searching && hasMostRecent ? reconstruct(mostRecent) : std::vector<PathNode>{};
}

void Pathfinder::finish(const SearchStatus newStatus, const BlockPos& end) {
    status = newStatus;
    result = reconstruct(end);
    const auto found = nodes.find(end);
    resultCost = found == nodes.end() ? 0.0 : found->second.g;
    open = {};
}

std::vector<PathNode> Pathfinder::reconstruct(const BlockPos& end) const {
    std::vector<PathNode> path;
    auto cursor = end;

    while (true) {
        const auto found = nodes.find(cursor);
        if (found == nodes.end())
            return {};

        path.push_back({cursor, found->second.movement, found->second.movementCost});
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
