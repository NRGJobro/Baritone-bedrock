#include "BaritoneController.h"

#include "Bedrock/BedrockWorld.h"
#include "Bedrock/PathRenderer.h"
#include "../SDK/MC.h"
#include "../SDK/Client/GUI/GuiData.h"
#include "../SDK/World/Actor/LocalPlayer.h"

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
        cells.push_back(source.offset(0, 1, 0));
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
    lastReplanReason.clear();
    protectedMiningSupports.clear();
    // Keep planning and execution on the same movement capability profile.
    options.preferSprint = executionOptions.sprint;
    const auto start = getPlayerBlock();
    // Ordinary navigation always proves that no natural route exists before
    // terrain breaking is considered. Mining mode still receives break moves
    // during this stage through routeOptions(), so explicit mining commands
    // retain their direct shaft behavior.
    routeStage = RouteStage::Walk;
    replanGuard = {};
    beginCalculation(start);
    return true;
}

bool BaritoneController::goTo(std::shared_ptr<Goal> newGoal) {
    setGoal(std::move(newGoal));
    return path();
}

void BaritoneController::stop() {
    pathNeedsContinuation = false;
    planningAhead = false;
    aheadRetryCooldown = 0;
    nextPlanningIndex = 0;
    pathfinder.cancel();
    executor.stop(MC::getLocalPlayer());
    protectedMiningSupports.clear();
    state = ControllerState::Idle;
}

void BaritoneController::resetForWorldChange() {
    pathNeedsContinuation = false;
    planningAhead = false;
    aheadRetryCooldown = 0;
    nextPlanningIndex = 0;
    pathfinder.cancel();
    executor.stop(nullptr);
    protectedMiningSupports.clear();
    goal.reset();
    replanGuard = {};
    routeStage = RouteStage::Walk;
    calculationStart = {};
    stuckReplans = 0;
    lastReplanReason.clear();
    stateBeforePause = ControllerState::Idle;
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
            // Let bounded A* finish choosing this segment before committing it.
            // Chaining early best-so-far prefixes locks in exploratory detours.
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
            pathNeedsContinuation = search == SearchStatus::Partial;
            planningAhead = false;
            aheadRetryCooldown = 0;
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

    if (aheadRetryCooldown > 0)
        --aheadRetryCooldown;

    if (pathNeedsContinuation && !planningAhead && aheadRetryCooldown == 0 &&
        goal && shouldPlanContinuation(executor.getCurrentIndex(),
            executor.getPath().size(), nextPlanningIndex)) {
        // The committed route never changes under the player. Every new search
        // starts at its endpoint, including searches following early previews.
        calculationStart = executor.getPath().back().pos;
        pathfinder.begin(calculationStart, goal, activeOptions);
        planningAhead = true;
    }

    if (planningAhead) {
        const BedrockWorld bedrockWorld(region);
        const ProtectedMiningWorld world(bedrockWorld, protectedMiningSupports);
        const auto search = pathfinder.step(world);
        const bool searching = search == SearchStatus::Searching;
        const auto& continuation = pathfinder.getPath();
        const bool ready = (search == SearchStatus::Found || search == SearchStatus::Partial) &&
            continuation.size() > 1;

        if (ready) {
            auto joined = executor.getPath();
            if (appendPathContinuation(joined, continuation)) {
                bool retryWithProtection = false;
                if (activeOptions.allowBreak) {
                    for (const auto& support : conflictingMiningSupports(joined))
                        retryWithProtection |= protectedMiningSupports.insert(support).second;
                }
                if (retryWithProtection) {
                    pathfinder.begin(calculationStart, goal, activeOptions);
                } else if (executor.extendIfPrefix(joined)) {
                    // Only one segment ahead. Even a short continuation must
                    // not trigger another search until we enter that segment.
                    nextPlanningIndex = joined.size() - continuation.size();
                    executor.updateCapabilities(activeOptions.allowBreak,
                        activeOptions.allowWater, activeOptions.bridgeOverWaterOnly,
                        !options.miningMode);
                    // Calculation rests while the prepared segment is followed.
                    pathfinder.cancel();
                    planningAhead = false;
                    pathNeedsContinuation = !goal->isInGoal(joined.back().pos);
                    aheadRetryCooldown = 0;
                }
            } else {
                pathfinder.cancel();
                planningAhead = false;
                aheadRetryCooldown = 5;
            }
        } else if (!searching) {
            planningAhead = false;
            aheadRetryCooldown = 5;
        }
    }

    switch (executor.tick(player, executionOptions)) {
    case ExecutionStatus::Running:
        break;
    case ExecutionStatus::Arrived:
        if (goal && goal->isInGoal(getPlayerBlock())) {
            pathfinder.cancel();
            pathNeedsContinuation = false;
            planningAhead = false;
            state = ControllerState::Arrived;
            message("Goal reached.");
        } else if (planningAhead) {
            // Executor has released movement at the validated endpoint. Keep
            // the in-flight search and append its result when ready.
            state = ControllerState::Executing;
        } else {
            // Re-evaluate natural terrain first after each completed segment.
            // A previous bridge/tunnel never enables construction permanently.
            routeStage = RouteStage::Walk;
            beginCalculation(getPlayerBlock());
        }
        break;
    case ExecutionStatus::Stuck:
    case ExecutionStatus::OffPath:
        lastReplanReason = executor.getLastFailureReason();
        lastReplanReason += player->isOnGround() ? " (ground)" : " (air)";
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
    auto* player = MC::getLocalPlayer();
    // Phase reapplies its cached command after ActorBaseTick because vanilla
    // repopulates MoveInputComponent during the original call. Keep the same
    // ordering so the normal movement systems see Limiter's W/A/S/D state.
    executor.reapplyInput(player);
    executor.applyVisualRotation(player);
}

void BaritoneController::suspendMovement() {
    executor.suspend(MC::getLocalPlayer());
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
    std::string result = stateName(state) + " [air-2]";
    if (goal)
        result += " | goal: " + goal->describe();
    if (state == ControllerState::Calculating)
        result += " | nodes: " + std::to_string(pathfinder.getExpandedNodeCount());
    if (state == ControllerState::Calculating && !lastReplanReason.empty())
        result += " | recovery: " + lastReplanReason;
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
    pathNeedsContinuation = false;
    planningAhead = false;
    aheadRetryCooldown = 0;
    nextPlanningIndex = 0;
    executor.stop(MC::getLocalPlayer());
    if (!goal || !replanGuard.allow(start, goal->heuristic(start))) {
        pathfinder.cancel();
        state = ControllerState::Failed;
        message("Stopped: repeated searches are not making progress.");
        return;
    }
    activeOptions = routeOptions(options, routeStage);
    // Keep the user's configured parkour range within the executor's supported
    // bounds. Longer jumps are aligned and accelerated by the runway controller
    // while movement remains transformed relative to the real camera.
    activeOptions.maxParkourDistance = cameraIndependentParkourDistance(
        activeOptions.maxParkourDistance);
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
