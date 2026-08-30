#pragma once

#include "Goal.h"
#include "Movement.h"

#include <memory>
#include <limits>
#include <queue>
#include <unordered_map>
#include <vector>

namespace baritone {

enum class SearchStatus {
    Idle,
    Searching,
    Found,
    Partial,
    Failed,
    Cancelled
};

struct PathNode {
    BlockPos pos;
    MovementType movement = MovementType::Start;
    double costFromPrevious = 0.0;
};

class Pathfinder {
    struct Node {
        double g = std::numeric_limits<double>::infinity();
        double f = std::numeric_limits<double>::infinity();
        BlockPos parent{};
        MovementType movement = MovementType::Start;
        double movementCost = 0.0;
        bool hasParent = false;
        bool closed = false;
    };

    struct QueueEntry {
        BlockPos pos;
        double f;
        double g;
    };

    struct QueueCompare {
        bool operator()(const QueueEntry& left, const QueueEntry& right) const {
            if (left.f == right.f)
                return left.g < right.g;
            return left.f > right.f;
        }
    };

    PathOptions options{};
    std::shared_ptr<Goal> goal{};
    BlockPos start{};
    BlockPos best{};
    BlockPos mostRecent{};
    double bestHeuristic = std::numeric_limits<double>::infinity();
    double bestScore = std::numeric_limits<double>::infinity();
    bool hasMostRecent = false;
    std::size_t expanded = 0;
    SearchStatus status = SearchStatus::Idle;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, QueueCompare> open;
    std::unordered_map<BlockPos, Node, BlockPosHash> nodes;
    // Movement generation asks about the same clearance/support cells many
    // times. Keep a per-search cache so Bedrock block lookups are not repeated
    // for every neighboring node expansion.
    std::unordered_map<BlockPos, BlockState, BlockPosHash> worldCache;
    std::vector<PathNode> result;
    double resultCost = 0.0;

    void finish(SearchStatus newStatus, const BlockPos& end);
    [[nodiscard]] std::vector<PathNode> reconstruct(const BlockPos& end) const;

public:
    void begin(const BlockPos& start, std::shared_ptr<Goal> goal, PathOptions options = {});
    SearchStatus step(const IWorld& world, std::size_t budget = 0);
    void cancel();

    [[nodiscard]] SearchStatus getStatus() const;
    [[nodiscard]] std::size_t getExpandedNodeCount() const;
    [[nodiscard]] const std::vector<PathNode>& getPath() const;
    [[nodiscard]] double getPathCost() const;
    [[nodiscard]] std::vector<PathNode> getBestPathSoFar() const;
    [[nodiscard]] std::vector<PathNode> getMostRecentPath() const;
};

} // namespace baritone
