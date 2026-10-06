#pragma once

#include "Bedrock/PathExecutor.h"
#include "Core/Goal.h"
#include "Core/NavigationPolicy.h"

#include <unordered_set>

namespace baritone {

enum class ControllerState {
    Idle,
    Calculating,
    Executing,
    Paused,
    Arrived,
    Failed
};

struct RenderOptions {
    bool renderPath = true;
    bool renderGoal = true;
    bool animatedGoal = true;
    bool renderCalculations = true;
    bool renderThroughWalls = true;
};

class BaritoneController {
    ControllerState state = ControllerState::Idle;
    ControllerState stateBeforePause = ControllerState::Idle;
    PathOptions options{};
    ExecutionOptions executionOptions{};
    RenderOptions renderOptions{};
    std::shared_ptr<Goal> goal{};
    Pathfinder pathfinder{};
    PathExecutor executor{};
    PathOptions activeOptions{};
    RouteStage routeStage = RouteStage::Walk;
    ReplanGuard replanGuard{};
    // Partial A* results are executable route segments. Plan the following
    // segment while the current one is still moving so their boundary does
    // not introduce a stop-and-replan pause.
    bool pathNeedsContinuation = false;
    bool planningAhead = false;
    int aheadRetryCooldown = 0;
    std::size_t nextPlanningIndex = 0;
    BlockPos calculationStart{};
    bool replanWhenStuck = true;
    int stuckReplans = 0;
    std::string lastReplanReason;
    // If a candidate mining route schedules a block for removal and later
    // needs that same block as footing, recalculate with that support treated
    // as non-breakable for the lifetime of this goal.
    std::unordered_set<BlockPos, BlockPosHash> protectedMiningSupports;

    [[nodiscard]] BlockPos getPlayerBlock() const;
    void beginCalculation(const BlockPos& start);
    bool tryFallback();
    void message(const std::string& text) const;

public:
    void setGoal(std::shared_ptr<Goal> goal);
    bool path();
    bool goTo(std::shared_ptr<Goal> goal);
    void stop();
    // Reset planner/executor state without dereferencing Minecraft objects.
    // Used when the current world/LocalPlayer may already have been destroyed.
    void resetForWorldChange();
    void pause();
    void resume();
    void tick();
    void postTick();
    void suspendMovement();
    void beginVisualRotationRender();
    void endVisualRotationRender();
    void render(const std::vector<BlockPos>& miningTargets = {},
        bool miningActive = false) const;

    [[nodiscard]] ControllerState getState() const;
    [[nodiscard]] std::string getStatusLine() const;
    [[nodiscard]] double getEstimatedTicksToGoal() const;
    [[nodiscard]] const std::shared_ptr<Goal>& getGoal() const;
    [[nodiscard]] PathOptions& getOptions();
    [[nodiscard]] ExecutionOptions& getExecutionOptions();
    [[nodiscard]] RenderOptions& getRenderOptions();
    [[nodiscard]] bool& getReplanWhenStuck();
};

} // namespace baritone
