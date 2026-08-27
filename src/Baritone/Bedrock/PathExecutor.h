#pragma once

#include "../Core/Pathfinder.h"

#include <glm/glm.hpp>

class LocalPlayer;

namespace baritone {

struct ExecutionOptions {
    bool sprint = true;
};

enum class ExecutionStatus {
    Idle,
    Running,
    Arrived,
    Stuck,
    NoPlayer
};

class PathExecutor {
    std::vector<PathNode> path;
    std::size_t index = 0;
    glm::vec3 lastProgressPosition{};
    int ticksWithoutProgress = 0;
    bool progressInitialized = false;
    bool controlledMovement = false;
    std::size_t activeParkourIndex = static_cast<std::size_t>(-1);
    bool parkourJumpIssued = false;
    bool parkourWasAirborne = false;
    bool parkourSprintPrimed = false;
    int parkourLaunchTicks = 0;
    bool movementModeCaptured = false;
    bool previousCameraRelativeMovement = true;
    bool previousRotationControlledByMovement = false;

    void clearInput(LocalPlayer* player);
    void resetParkourState();

public:
    void begin(std::vector<PathNode> path);
    ExecutionStatus tick(LocalPlayer* player, const ExecutionOptions& options);
    void suspend(LocalPlayer* player);
    void stop(LocalPlayer* player);

    [[nodiscard]] std::size_t getCurrentIndex() const;
    [[nodiscard]] const std::vector<PathNode>& getPath() const;
};

} // namespace baritone
