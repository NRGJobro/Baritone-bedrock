#pragma once

#include "../Core/BlockPos.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

class BlockLegacy;

namespace baritone {

class BaritoneController;

// Bedrock counterpart to Baritone's MineProcess. The process scans the loaded
// world for matching runtime block IDs, paths to a reachable face when needed,
// directly mines in-range matches through intervening blocks, and repeats
// until the requested count.
class MiningProcess {
    struct TrackedDropActor {
        std::uint32_t entityId = 0;
        BlockPos source;
    };

    std::vector<int> blockIds;
    std::vector<std::string> blockNames;
    std::vector<BlockPos> candidates;
    // Connected ore blocks around the current goal. Keeping this patch alive
    // avoids rebuilding a composite goal after every block in a vein breaks.
    std::vector<BlockPos> activePatch;
    std::vector<BlockPos> fixedTargets;
    std::unordered_set<BlockPos, BlockPosHash> blacklist;
    std::optional<BlockPos> noVisibleTargetPosition;
    // The single matching block the current route is trying to reach. Mining
    // deliberately paths to one nearest target at a time instead of giving A*
    // a composite goal that may select a farther member of the ore patch.
    std::optional<BlockPos> pathingTarget;
    std::optional<BlockPos> breakingTarget;
    // Drops are collected as a batch after every immediately reachable block
    // in the current patch has been mined. This keeps vein mining smooth while
    // preventing the process from leaving the area with items on the floor.
    std::vector<BlockPos> pendingPickupTargets;
    std::vector<TrackedDropActor> pendingPickupActors;
    std::unordered_set<std::uint32_t> knownActorIds;
    std::optional<TrackedDropActor> pickupActor;
    std::optional<BlockPos> pickupTarget;
    std::optional<BlockPos> pickupGoal;
    bool breakingTargetCountsGoal = true;
    bool pendingCompletion = false;
    bool collectingDrops = false;
    bool actorSnapshotInitialized = false;
    int pickupTicks = 0;
    int pickupPathAttempts = 0;
    std::optional<std::string> pendingMessage;
    int desiredQuantity = 0;
    int minedQuantity = 0;
    int scanRadius = 24;
    int breakTicks = 0;
    int previousHotbarSlot = -1;
    bool active = false;
    bool fixedTargetMode = false;
    bool continueMining = false;
    bool ownsBreakPolicy = false;
    bool previousAllowBreak = false;
    bool previousAllowWater = true;
    bool previousAllowParkour = true;
    bool previousAllowParkourAscend = true;
    bool previousAllowBridge = false;
    bool previousBridgeOverWaterOnly = false;
    bool previousBridgeOnlyAfterFailure = true;
    bool previousMiningMode = false;
    bool previousPreferVerticalMining = false;
    int previousPreferredVerticalMiningY = std::numeric_limits<int>::min();
    std::size_t previousMaxExpandedNodes = 60000;
    std::size_t previousNodesPerTick = 350;
    double previousHeuristicWeight = 1.0;

    [[nodiscard]] bool matches(::BlockLegacy* block) const;
    [[nodiscard]] std::vector<BlockPos> scan() const;
    [[nodiscard]] std::vector<BlockPos> collectPatch(const BlockPos& seed,
        const std::vector<BlockPos>& source) const;
    void beginBreaking(const BlockPos& target);
    void selectBestTool(const BlockPos& target);
    void restoreHotbar();
    void enableBreakPolicy(BaritoneController& controller);
    void restoreBreakPolicy(BaritoneController& controller);

public:
    void start(std::vector<int> ids, std::vector<std::string> names, int quantity, int radius = 24,
        bool continueMode = false);
    void startTargets(std::vector<BlockPos> targets);
    void cancel(BaritoneController& controller);
    void tick(BaritoneController& controller);

    [[nodiscard]] bool isActive() const;
    [[nodiscard]] int getMinedQuantity() const;
    [[nodiscard]] int getDesiredQuantity() const;
    [[nodiscard]] std::string getStatusLine() const;
    [[nodiscard]] std::vector<BlockPos> getRenderTargets() const;
    std::optional<std::string> takeMessage();
};

} // namespace baritone
