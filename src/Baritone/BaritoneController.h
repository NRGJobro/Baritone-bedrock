#pragma once

#include "Bedrock/PathExecutor.h"
#include "Core/Goal.h"

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
    bool fadePath = false;
    bool pathAsLine = false;
    bool renderCalculations = true;
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
    bool replanWhenStuck = true;
    int stuckReplans = 0;

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
    void render() const;

    [[nodiscard]] ControllerState getState() const;
    [[nodiscard]] std::string getStatusLine() const;
    [[nodiscard]] const std::shared_ptr<Goal>& getGoal() const;
    [[nodiscard]] PathOptions& getOptions();
    [[nodiscard]] ExecutionOptions& getExecutionOptions();
    [[nodiscard]] RenderOptions& getRenderOptions();
    [[nodiscard]] bool& getReplanWhenStuck();
};

} // namespace baritone
