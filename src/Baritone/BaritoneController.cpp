#include "BaritoneController.h"

#include "Bedrock/BedrockWorld.h"
#include "Bedrock/PathRenderer.h"
#include "../SDK/MC.h"

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
    // Keep planning and execution on the same movement capability profile.
    options.preferSprint = executionOptions.sprint;
    options.bridgeOnlyAfterFailure = options.allowBridge;
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
        BedrockWorld world(region);
        const auto search = pathfinder.step(world);
        if (search == SearchStatus::Searching)
            return;

        if (search == SearchStatus::Partial && options.allowBridge && options.bridgeOnlyAfterFailure) {
            options.bridgeOnlyAfterFailure = false;
            // Construction expands the frontier substantially; give the
            // fallback search enough budget to reach the far platform instead
            // of stopping at the same loaded-world partial edge.
            options.maxExpandedNodes = std::max<std::size_t>(options.maxExpandedNodes, 200000);
            message("Ordinary search reached a dead end; evaluating bridge routes.");
            beginCalculation(getPlayerBlock());
            return;
        }
        if ((search == SearchStatus::Found || search == SearchStatus::Partial) && pathfinder.getPath().size() > 1) {
            executingPartialPath = search == SearchStatus::Partial;
            executor.begin(pathfinder.getPath());
            state = ControllerState::Executing;
            message(executingPartialPath ? "Using a partial path to the loaded-world edge." : "Path found.");
        } else if (search == SearchStatus::Found && pathfinder.getPath().size() == 1) {
            state = ControllerState::Arrived;
            message("Already at the goal.");
        } else {
            if (options.allowBridge && options.bridgeOnlyAfterFailure) {
                options.bridgeOnlyAfterFailure = false;
                options.maxExpandedNodes = std::max<std::size_t>(options.maxExpandedNodes, 200000);
                message("No ordinary route; evaluating bridge routes.");
                beginCalculation(getPlayerBlock());
                return;
            }
            state = ControllerState::Failed;
            message("No walkable path was found from the current position.");
        }
        return;
    }

    if (state != ControllerState::Executing)
        return;

    switch (executor.tick(player, executionOptions)) {
    case ExecutionStatus::Running:
        break;
    case ExecutionStatus::Arrived:
        if (executingPartialPath && goal && !goal->isInGoal(getPlayerBlock())) {
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

void BaritoneController::render() const {
    const auto bestPath = renderOptions.renderCalculations ? pathfinder.getBestPathSoFar() : std::vector<PathNode>{};
    const auto recentPath = renderOptions.renderCalculations ? pathfinder.getMostRecentPath() : std::vector<PathNode>{};
    PathRenderer::render(executor.getPath(), executor.getCurrentIndex(), bestPath, recentPath, goal.get(), renderOptions);
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
    state = ControllerState::Calculating;
}

void BaritoneController::message(const std::string& text) const {
    if (const auto gui = MC::getGuiData())
        gui->displayClientMessage("\xC2\xA7" "6[Limiter]" "\xC2\xA7" "r " + text);
}

} // namespace baritone
