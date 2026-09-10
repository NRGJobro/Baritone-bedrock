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
    protectedMiningSupports.clear();
    // Keep planning and execution on the same movement capability profile.
    options.preferSprint = executionOptions.sprint;
    routeStage = RouteStage::Walk;
    replanGuard = {};
    beginCalculation(getPlayerBlock());
    return true;
}

bool BaritoneController::goTo(std::shared_ptr<Goal> newGoal) {
    setGoal(std::move(newGoal));
    return path();
}

void BaritoneController::stop() {
    executingPreview = false;
    pathfinder.cancel();
    executor.stop(MC::getLocalPlayer());
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
            auto preview = pathSuffix(pathfinder.getBestPathSoFar(), getPlayerBlock(), true);
            if (preview.size() > 1) {
                executor.begin(std::move(preview), false, activeOptions.allowWater);
                executingPreview = true;
                state = ControllerState::Executing;
            }
            return;
        }

        const auto path = pathSuffix(pathfinder.getPath(), getPlayerBlock());
        if (search == SearchStatus::Partial && path.size() == 1 && getPlayerBlock() != calculationStart) {
            // Reaching a temporary frontier is progress, not evidence that
            // walking failed. Continue naturally instead of enabling building.
            routeStage = RouteStage::Walk;
            beginCalculation(getPlayerBlock());
            return;
        }
        if ((search == SearchStatus::Found || search == SearchStatus::Partial) && path.empty()) {
            beginCalculation(getPlayerBlock());
            return;
        }
        if (search == SearchStatus::Found || (search == SearchStatus::Partial && path.size() > 1)) {
            if (activeOptions.allowBreak && path.size() > 1) {
                const auto conflicts = conflictingMiningSupports(path);
                bool addedProtection = false;
                for (const auto& support : conflicts)
                    addedProtection |= protectedMiningSupports.insert(support).second;
                if (addedProtection) {
                    beginCalculation(getPlayerBlock());
                    return;
                }
            }
            if (path.size() == 1) {
                state = ControllerState::Arrived;
                message("Already at the goal.");
                return;
            }
            executor.begin(path, activeOptions.allowBreak, activeOptions.allowWater,
                activeOptions.bridgeOverWaterOnly, !options.miningMode);
            executingPreview = false;
            state = ControllerState::Executing;
            return;
        }
        if (tryFallback())
            return;
        state = ControllerState::Failed;
        if (!options.miningMode)
            message("No safe route found within the search limit. Navigation stopped.");
        return;
    }

    if (state != ControllerState::Executing)
        return;

    if (executingPreview) {
        const BedrockWorld bedrockWorld(region);
        const ProtectedMiningWorld world(bedrockWorld, protectedMiningSupports);
        const auto search = pathfinder.step(world, 32);
        const auto candidate = search == SearchStatus::Searching
            ? pathfinder.getBestPathSoFar() : pathfinder.getPath();
        if (!executor.getPath().empty()) {
            const auto extension = pathSuffix(candidate, executor.getPath().front().pos, true);
            (void)executor.extendIfPrefix(extension);
        }
    }

    switch (executor.tick(player, executionOptions)) {
    case ExecutionStatus::Running:
        break;
    case ExecutionStatus::Arrived:
        if (goal && goal->isInGoal(getPlayerBlock())) {
            pathfinder.cancel();
            state = ControllerState::Arrived;
            message("Goal reached.");
        } else if (executingPreview) {
            // Keep the same bounded search alive if the frontier diverged.
            // The next tick adopts a suffix or waits for this search to finish;
            // it does not restart A* every time a short preview ends.
            executingPreview = false;
            state = ControllerState::Calculating;
        } else {
            // Re-evaluate natural terrain first after each completed segment.
            // A previous bridge/tunnel never enables construction permanently.
            routeStage = RouteStage::Walk;
            beginCalculation(getPlayerBlock());
        }
        break;
    case ExecutionStatus::Stuck:
    case ExecutionStatus::OffPath:
        if (replanWhenStuck && ++stuckReplans <= 3) {
            routeStage = RouteStage::Walk;
            beginCalculation(getPlayerBlock());
        } else {
            executor.stop(player);
            pathfinder.cancel();
            state = ControllerState::Failed;
            message("Stopped after repeated movement failures.");
        }
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
        static_cast<int>(std::floor(feet.y + 0.1251f)),
        static_cast<int>(std::floor(feet.z))
    };
}

void BaritoneController::beginCalculation(const BlockPos& start) {
    executingPreview = false;
    executor.stop(MC::getLocalPlayer());
    if (!goal || !replanGuard.allow(start, goal->heuristic(start))) {
        pathfinder.cancel();
        state = ControllerState::Failed;
        message("Stopped: repeated searches are not making progress.");
        return;
    }
    activeOptions = routeOptions(options, routeStage);
    calculationStart = start;
    pathfinder.begin(start, goal, activeOptions);
    state = ControllerState::Calculating;
}

bool BaritoneController::tryFallback() {
    if (routeStage == RouteStage::Walk && !options.miningMode) {
        routeStage = RouteStage::Break;
    } else if (routeStage != RouteStage::Build && options.allowBridge) {
        routeStage = RouteStage::Build;
    } else {
        return false;
    }
    beginCalculation(getPlayerBlock());
    return true;
}
void BaritoneController::message(const std::string& text) const {
    if (const auto gui = MC::getGuiData())
        gui->displayClientMessage("\xC2\xA7" "6[Limiter]" "\xC2\xA7" "r " + text);
}

} // namespace baritone
