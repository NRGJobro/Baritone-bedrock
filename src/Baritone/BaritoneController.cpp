#include "BaritoneController.h"

#include "Bedrock/BedrockWorld.h"
#include "Bedrock/PathRenderer.h"
#include "../SDK/MC.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace baritone {
namespace {

std::string stateName(const ControllerState state) {
    switch (state) {
    case ControllerState::Idle: return "idle";
    case ControllerState::Calculating: return "calculating";
    case ControllerState::Executing: return "executing";
    case ControllerState::Paused: return "paused";
    case ControllerState::Arrived: return "arrived";
    case ControllerState::Failed: return "failed";
    }
    return "unknown";
}

class ProtectedMiningWorld final : public IWorld {
    const IWorld& source;
    const std::unordered_set<BlockPos, BlockPosHash>& protectedSupports;

public:
    ProtectedMiningWorld(const IWorld& source,
        const std::unordered_set<BlockPos, BlockPosHash>& protectedSupports)
        : source(source), protectedSupports(protectedSupports) {}

    [[nodiscard]] BlockState getBlock(const BlockPos& pos) const override {
        auto state = source.getBlock(pos);
        if (state.solid && protectedSupports.contains(pos))
            state.breakable = false;
        return state;
    }
};

void plannedBreakCells(const std::vector<PathNode>& path, const std::size_t index,
    std::vector<BlockPos>& cells) {
    if (index == 0 || index >= path.size())
        return;
    const auto addColumn = [&](const BlockPos& feet) {
        cells.push_back(feet);
        cells.push_back(feet.offset(0, 1, 0));
    };
    const auto& node = path[index];
    const auto& source = path[index - 1].pos;
    const int dx = std::clamp(node.pos.x - source.x, -1, 1);
    const int dz = std::clamp(node.pos.z - source.z, -1, 1);
    if (node.movement == MovementType::BreakTraverse ||
        node.movement == MovementType::BreakAscend) {
        if (node.movement == MovementType::BreakAscend)
            cells.push_back(source.offset(0, 2, 0));
        addColumn(node.pos);
    } else if (node.movement == MovementType::BreakDown) {
        cells.push_back(node.pos);
    } else if (node.movement == MovementType::BreakDescend) {
        addColumn(source.offset(dx, 0, dz));
        addColumn(node.pos);
    }
}

std::unordered_set<BlockPos, BlockPosHash> conflictingMiningSupports(
    const std::vector<PathNode>& path) {
    std::unordered_set<BlockPos, BlockPosHash> futureSupports;
    std::unordered_set<BlockPos, BlockPosHash> conflicts;
    if (path.size() < 2)
        return conflicts;

    std::vector<BlockPos> breakCells;
    breakCells.reserve(4);
    for (std::size_t index = path.size() - 1; index > 0; --index) {
        // The source support is required while the block is being mined; the
        // destination and accumulated later supports are required afterward.
        futureSupports.insert(path[index].pos.offset(0, -1, 0));
        // BreakDown deliberately removes the current footing only after the
        // player is centered over it. Other transitions must preserve their
        // source support throughout the movement.
        if (path[index].movement != MovementType::BreakDown)
            futureSupports.insert(path[index - 1].pos.offset(0, -1, 0));
        breakCells.clear();
        plannedBreakCells(path, index, breakCells);
        for (const auto& cell : breakCells) {
            if (futureSupports.contains(cell))
                conflicts.insert(cell);
        }
    }
    return conflicts;
}

std::vector<PathNode> suffixFromPosition(const std::vector<PathNode>& path,
    const BlockPos& position) {
    const auto current = std::ranges::find(path, position, &PathNode::pos);
    if (current == path.end())
        return {};

    std::vector<PathNode> suffix(current, path.end());
    if (!suffix.empty()) {
        suffix.front().movement = MovementType::Start;
        suffix.front().costFromPrevious = 0.0;
    }
    return suffix;
}

} // namespace

void BaritoneController::setGoal(std::shared_ptr<Goal> newGoal) {
    stop();
    goal = std::move(newGoal);
    state = ControllerState::Idle;
}

bool BaritoneController::path() {
    if (!goal || MC::getLocalPlayer() == nullptr || MC::getRegion() == nullptr)
        return false;

    stuckReplans = 0;
    supportConflictReplans = 0;
    protectedMiningSupports.clear();
    // Keep planning and execution on the same movement capability profile.
    options.preferSprint = executionOptions.sprint;
    // Mining enables only the inexpensive water-bridge generator and needs it
    // in the first bounded search. Ordinary navigation retains the broader
    // bridge fallback after a construction-free attempt.
    options.bridgeOnlyAfterFailure = options.allowBridge && !options.miningMode;
    beginCalculation(getPlayerBlock());
    return true;
}

bool BaritoneController::goTo(std::shared_ptr<Goal> newGoal) {
    setGoal(std::move(newGoal));
    return path();
}

void BaritoneController::stop() {
    pathfinder.cancel();
    executor.stop(MC::getLocalPlayer());
    executingPartialPath = false;
    executingPreviewPath = false;
    supportConflictReplans = 0;
    protectedMiningSupports.clear();
    state = ControllerState::Idle;
}

void BaritoneController::pause() {
    if (state != ControllerState::Calculating && state != ControllerState::Executing)
        return;
    stateBeforePause = state;
    executor.suspend(MC::getLocalPlayer());
    state = ControllerState::Paused;
}

void BaritoneController::resume() {
    if (state != ControllerState::Paused)
        return;

    if (stateBeforePause == ControllerState::Calculating)
        beginCalculation(getPlayerBlock());
    else if (stateBeforePause == ControllerState::Executing)
        beginCalculation(getPlayerBlock());
}

void BaritoneController::tick() {
    const auto player = MC::getLocalPlayer();
    const auto region = MC::getRegion();
    if (player == nullptr || region == nullptr) {
        if (state == ControllerState::Executing)
            executor.stop(player);
        return;
    }

    if (state == ControllerState::Calculating) {
        const BedrockWorld bedrockWorld(region);
        const ProtectedMiningWorld world(bedrockWorld, protectedMiningSupports);
        const auto search = pathfinder.step(world);
        if (search == SearchStatus::Searching) {
            // The dark-blue route is already a valid path to A*'s current best
            // frontier. Walk that snapshot now instead of idling until the
            // entire bounded search finishes. The pathfinder remains active
            // and its later common-prefix results extend the executor below.
            const auto preview = suffixFromPosition(pathfinder.getBestPathSoFar(), getPlayerBlock());
            if (preview.size() > 1) {
                if (options.miningMode) {
                    const auto conflicts = conflictingMiningSupports(preview);
                    bool addedProtection = false;
                    for (const auto& support : conflicts)
                        addedProtection |= protectedMiningSupports.insert(support).second;
                    if (addedProtection) {
                        if (++supportConflictReplans <= 3)
                            beginCalculation(getPlayerBlock());
                        else
                            state = ControllerState::Failed;
                        return;
                    }
                }
                executor.begin(preview, options.allowBreak, options.allowWater,
                    options.bridgeOverWaterOnly);
                executingPreviewPath = true;
                executingPartialPath = false;
                state = ControllerState::Executing;
            }
            return;
        }

        if (search == SearchStatus::Partial && options.allowBridge && options.bridgeOnlyAfterFailure &&
            !options.miningMode) {
            options.bridgeOnlyAfterFailure = false;
            // Construction expands the frontier substantially; give the
            // fallback search enough budget to reach the far platform instead
            // of stopping at the same loaded-world partial edge.
            options.maxExpandedNodes = std::max<std::size_t>(options.maxExpandedNodes, 200000);
            message("Ordinary search reached a dead end; evaluating bridge routes.");
            beginCalculation(getPlayerBlock());
            return;
        }
        if (options.miningMode &&
            (search == SearchStatus::Found || search == SearchStatus::Partial) &&
            pathfinder.getPath().size() > 1) {
            const auto conflicts = conflictingMiningSupports(pathfinder.getPath());
            bool addedProtection = false;
            for (const auto& support : conflicts)
                addedProtection |= protectedMiningSupports.insert(support).second;
            if (addedProtection) {
                // Re-run once with every conflicting step block preserved.
                // The next search may still stand on these blocks, but it can
                // no longer schedule them as tunnel clearance first.
                if (++supportConflictReplans <= 3) {
                    beginCalculation(getPlayerBlock());
                } else {
                    state = ControllerState::Failed;
                }
                return;
            }
        }
        if ((search == SearchStatus::Found || search == SearchStatus::Partial) && pathfinder.getPath().size() > 1) {
            executingPartialPath = search == SearchStatus::Partial;
            executingPreviewPath = false;
            executor.begin(pathfinder.getPath(), options.allowBreak, options.allowWater,
                options.bridgeOverWaterOnly);
            state = ControllerState::Executing;
            message(executingPartialPath ? "Using a partial path to the loaded-world edge." : "Path found.");
        } else if (search == SearchStatus::Found && pathfinder.getPath().size() == 1) {
            state = ControllerState::Arrived;
            message("Already at the goal.");
        } else {
            if (options.allowBridge && options.bridgeOnlyAfterFailure && !options.miningMode) {
                options.bridgeOnlyAfterFailure = false;
                options.maxExpandedNodes = std::max<std::size_t>(options.maxExpandedNodes, 200000);
                message("No ordinary route; evaluating bridge routes.");
                beginCalculation(getPlayerBlock());
                return;
            }
            state = ControllerState::Failed;
            // MiningProcess handles a blocked ore patch immediately and emits
            // a more useful patch-specific status. Avoid flashing a generic
            // failure immediately before its next focused tunnel search.
            if (!options.miningMode)
                message("No walkable path was found from the current position.");
        }
        return;
    }

    if (state != ControllerState::Executing)
        return;

    if (executingPreviewPath) {
        const BedrockWorld bedrockWorld(region);
        const ProtectedMiningWorld world(bedrockWorld, protectedMiningSupports);
        // Once movement has started, keep background planning below a small
        // fixed slice so it cannot monopolize a render/game tick.
        const auto backgroundBudget = std::min<std::size_t>(options.nodesPerTick, 64);
        const auto search = pathfinder.step(world, backgroundBudget);

        const auto& candidate = search == SearchStatus::Searching ?
            pathfinder.getBestPathSoFar() : pathfinder.getPath();
        if (candidate.size() > 1 && options.miningMode) {
            const auto conflicts = conflictingMiningSupports(candidate);
            bool addedProtection = false;
            for (const auto& support : conflicts)
                addedProtection |= protectedMiningSupports.insert(support).second;
            if (addedProtection) {
                if (++supportConflictReplans <= 3)
                    beginCalculation(getPlayerBlock());
                else {
                    executor.stop(player);
                    state = ControllerState::Failed;
                }
                return;
            }
        }

        if (search == SearchStatus::Partial && options.allowBridge &&
            options.bridgeOnlyAfterFailure && !options.miningMode) {
            options.bridgeOnlyAfterFailure = false;
            options.maxExpandedNodes = std::max<std::size_t>(options.maxExpandedNodes, 200000);
            message("Ordinary search reached a dead end; evaluating bridge routes.");
            beginCalculation(getPlayerBlock());
            return;
        }

        if (search == SearchStatus::Searching) {
            // Most weighted-A* frontier updates retain the route already being
            // followed. Append those nodes in place so reaching the old blue
            // endpoint does not introduce a stop or reset movement state.
            (void)executor.extendIfPrefix(candidate);
        } else if ((search == SearchStatus::Found || search == SearchStatus::Partial) &&
            candidate.size() > 1 && executor.extendIfPrefix(candidate)) {
            executingPreviewPath = false;
            executingPartialPath = search == SearchStatus::Partial;
            message(executingPartialPath ? "Using a partial path to the loaded-world edge." : "Path found.");
        }
    }

    switch (executor.tick(player, executionOptions)) {
    case ExecutionStatus::Running:
        break;
    case ExecutionStatus::Arrived:
        if (executingPreviewPath) {
            const auto playerBlock = getPlayerBlock();
            const auto search = pathfinder.getStatus();
            const auto candidate = search == SearchStatus::Searching ?
                pathfinder.getBestPathSoFar() : pathfinder.getPath();
            auto continuation = suffixFromPosition(candidate, playerBlock);
            if (continuation.size() > 1) {
                executor.begin(std::move(continuation), options.allowBreak, options.allowWater,
                    options.bridgeOverWaterOnly);
                if (search != SearchStatus::Searching) {
                    executingPreviewPath = false;
                    executingPartialPath = search == SearchStatus::Partial;
                }
            } else if (search == SearchStatus::Searching) {
                // The frontier changed before this snapshot ended. Restart A*
                // from the physical endpoint instead of steering backward to
                // the old search origin.
                beginCalculation(playerBlock);
            } else if (goal && goal->isInGoal(playerBlock)) {
                executingPreviewPath = false;
                state = ControllerState::Arrived;
                message("Goal reached.");
            } else {
                beginCalculation(playerBlock);
            }
        } else if (executingPartialPath && goal && !goal->isInGoal(getPlayerBlock())) {
            beginCalculation(getPlayerBlock());
        } else {
            state = ControllerState::Arrived;
            message("Goal reached.");
            
        }
        break;
    case ExecutionStatus::Stuck:
        if (replanWhenStuck && ++stuckReplans <= 3) {
            message("Movement stalled; recalculating.");
            beginCalculation(getPlayerBlock());
        } else {
            executor.stop(player);
            state = ControllerState::Failed;
            message("Stopped after three failed recovery attempts.");
        }
        break;
    case ExecutionStatus::OffPath:
        // Route invalidation is not a stall and should not consume one of the
        // limited stall-recovery attempts. Rebuild from the physical position.
        message("Left the path; finding a new route.");
        beginCalculation(getPlayerBlock());
        break;
    case ExecutionStatus::NoPlayer:
        executor.stop(player);
        state = ControllerState::Failed;
        break;
    case ExecutionStatus::Idle:
        break;
    }
}

void BaritoneController::postTick() {
    executor.applyVisualRotation(MC::getLocalPlayer());
}

void BaritoneController::beginVisualRotationRender() {
    executor.beginVisualRotationRender(MC::getLocalPlayer());
}

void BaritoneController::endVisualRotationRender() {
    executor.endVisualRotationRender(MC::getLocalPlayer());
}

void BaritoneController::render(const std::vector<BlockPos>& miningTargets,
    const bool miningActive) const {
    const auto bestPath = renderOptions.renderCalculations ? pathfinder.getBestPathSoFar() : std::vector<PathNode>{};
    const auto recentPath = renderOptions.renderCalculations ? pathfinder.getMostRecentPath() : std::vector<PathNode>{};
    PathRenderer::render(executor.getPath(), executor.getCurrentIndex(), bestPath,
        recentPath, goal.get(), miningTargets, miningActive, renderOptions);
}

ControllerState BaritoneController::getState() const { return state; }

std::string BaritoneController::getStatusLine() const {
    std::string result = stateName(state);
    if (goal)
        result += " | goal: " + goal->describe();
    if (state == ControllerState::Calculating)
        result += " | nodes: " + std::to_string(pathfinder.getExpandedNodeCount());
    if (state == ControllerState::Executing)
        result += " | node: " + std::to_string(executor.getCurrentIndex()) + "/" + std::to_string(executor.getPath().size());
    if (state == ControllerState::Executing && executingPreviewPath)
        result += " | searching: " + std::to_string(pathfinder.getExpandedNodeCount()) + " nodes";
    return result;
}

double BaritoneController::getEstimatedTicksToGoal() const {
    if (state == ControllerState::Executing)
        return executor.getEstimatedTicksRemaining();
    if (state == ControllerState::Calculating) {
        const auto bestPath = pathfinder.getBestPathSoFar();
        double cost = 0.0;
        for (const auto& node : bestPath)
            cost += node.costFromPrevious;
        return cost;
    }
    return 0.0;
}

const std::shared_ptr<Goal>& BaritoneController::getGoal() const { return goal; }

PathOptions& BaritoneController::getOptions() { return options; }

ExecutionOptions& BaritoneController::getExecutionOptions() { return executionOptions; }

RenderOptions& BaritoneController::getRenderOptions() { return renderOptions; }

bool& BaritoneController::getReplanWhenStuck() { return replanWhenStuck; }

BlockPos BaritoneController::getPlayerBlock() const {
    const auto player = MC::getLocalPlayer();
    if (player == nullptr)
        return {};
    const auto feet = player->getFeetPosition();
    return {
        static_cast<int>(std::floor(feet.x)),
        static_cast<int>(std::round(feet.y)),
        static_cast<int>(std::floor(feet.z))
    };
}

void BaritoneController::beginCalculation(const BlockPos& start) {
    executor.stop(MC::getLocalPlayer());
    pathfinder.begin(start, goal, options);
    executingPartialPath = false;
    executingPreviewPath = false;
    state = ControllerState::Calculating;
}

void BaritoneController::message(const std::string& text) const {
    if (const auto gui = MC::getGuiData())
        gui->displayClientMessage("\xC2\xA7" "6[Limiter]" "\xC2\xA7" "r " + text);
}

} // namespace baritone
