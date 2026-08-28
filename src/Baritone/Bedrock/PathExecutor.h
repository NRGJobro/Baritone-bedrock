#pragma once

#include "../Core/Pathfinder.h"
#include "../../SDK/World/Level/HitResult/FacingID.h"

#include <glm/glm.hpp>

class LocalPlayer;

namespace baritone {

struct ExecutionOptions {
    bool sprint = true;
    float rotationSmoothness = 4.5f;
};

enum class ExecutionStatus {
    Idle,
    Running,
    Arrived,
    Stuck,
    OffPath,
    NoPlayer
};

class PathExecutor {
    std::vector<PathNode> path;
    std::size_t index = 0;
    glm::vec3 lastProgressPosition{};
    int ticksWithoutProgress = 0;
    int ticksOutsidePath = 0;
    bool progressInitialized = false;
    bool controlledMovement = false;
    std::size_t activeParkourIndex = static_cast<std::size_t>(-1);
    bool parkourJumpIssued = false;
    bool parkourWasAirborne = false;
    bool parkourSprintPrimed = false;
    int parkourSprintTicks = 0;
    int parkourLaunchTicks = 0;
    bool movementModeCaptured = false;
    bool previousCameraRelativeMovement = true;
    bool previousRotationControlledByMovement = false;
    bool pathRotationActive = false;
    bool visualYawInitialized = false;
    float pathMovementYaw = 0.f;
    float visualYaw = 0.f;
    float visualBodyYaw = 0.f;
    float cameraYaw = 0.f;
    bool cameraYawCaptured = false;
    bool bridgePitchActive = false;
    float savedBridgePitch = 0.f;
    float bridgePitch = 0.f;
    float bridgePitchTarget = 62.f;
    float visualPitch = 0.f;
    glm::vec2 savedActorRotation{};
    bool renderRotationOverrideActive = false;
    glm::vec2 savedHeadRotation{};
    float savedBodyRotation = 0.f;
    float savedPreviousBodyRotation = 0.f;
    std::uint64_t lastRotationRenderMillis = 0;
    float rotationSmoothness = 1.f;
    int bridgeNextStep = 1;
    int bridgePlacementWait = 0;
    std::size_t activeBridgeIndex = static_cast<std::size_t>(-1);
    bool waterDescentActive = false;
    int waterColumnX = 0;
    int waterColumnZ = 0;
    int waterBottomY = 0;
    bool waterBottomKnown = false;
    int waterPathBottomY = 0;
    std::size_t waterExitIndex = static_cast<std::size_t>(-1);
    int waterDescentTicks = 0;
    int waterVerticalSettledTicks = 0;
    bool waterHasDescended = false;
    float lastWaterFeetY = 0.f;
    float waterFacingYaw = 0.f;

    void clearInput(LocalPlayer* player);
    void resetParkourState();
    bool placeBridgeBlock(LocalPlayer* player, const BlockPos& target, FacingID preferredFace = FacingID::Unknown);

public:
    void begin(std::vector<PathNode> path);
    ExecutionStatus tick(LocalPlayer* player, const ExecutionOptions& options);
    void applyVisualRotation(LocalPlayer* player);
    void beginVisualRotationRender(LocalPlayer* player);
    void endVisualRotationRender(LocalPlayer* player);
    void suspend(LocalPlayer* player);
    void stop(LocalPlayer* player);

    [[nodiscard]] std::size_t getCurrentIndex() const;
    [[nodiscard]] const std::vector<PathNode>& getPath() const;
};

} // namespace baritone
