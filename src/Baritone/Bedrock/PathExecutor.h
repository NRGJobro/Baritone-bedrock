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

    void clearInput(LocalPlayer* player);

public:
    void begin(std::vector<PathNode> path);
    ExecutionStatus tick(LocalPlayer* player, const ExecutionOptions& options);
    void suspend(LocalPlayer* player);
    void stop(LocalPlayer* player);

    [[nodiscard]] std::size_t getCurrentIndex() const;
    [[nodiscard]] const std::vector<PathNode>& getPath() const;
};

} // namespace baritone
