#pragma once

#include "Bedrock/PathExecutor.h"
#include "Core/Goal.h"

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
    bool executingPartialPath = false;
    // The executor may walk a best-so-far path while the same A* search keeps
    // expanding in the background.
    bool executingPreviewPath = false;
    bool replanWhenStuck = true;
    int stuckReplans = 0;
    int supportConflictReplans = 0;
    // If a candidate mining route schedules a block for removal and later
    // needs that same block as footing, recalculate with that support treated
    // as non-breakable for the lifetime of this goal.
    std::unordered_set<BlockPos, BlockPosHash> protectedMiningSupports;

    [[nodiscard]] BlockPos getPlayerBlock() const;
    void beginCalculation(const BlockPos& start);
    void message(const std::string& text) const;

public:
    void setGoal(std::shared_ptr<Goal> goal);
    bool path();
    bool goTo(std::shared_ptr<Goal> goal);
    void stop();
    void pause();
    void resume();
    void tick();
    void postTick();
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
