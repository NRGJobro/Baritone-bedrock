#include "PathExecutor.h"

#include "../../SDK/Client/Input/MoveInputComponent.h"
#include "../../SDK/World/Actor/LocalPlayer.h"

#include <numbers>

namespace baritone {
namespace {

float horizontalDistance(const glm::vec3& left, const glm::vec3& right) {
    const float dx = left.x - right.x;
    const float dz = left.z - right.z;
    return std::sqrt(dx * dx + dz * dz);
}

} // namespace

void PathExecutor::begin(std::vector<PathNode> newPath) {
    path = std::move(newPath);
    index = path.size() > 1 ? 1 : path.size();
    lastProgressPosition = {};
    ticksWithoutProgress = 0;
    progressInitialized = false;
    controlledMovement = false;
    resetParkourState();
}

ExecutionStatus PathExecutor::tick(LocalPlayer* player, const ExecutionOptions& options) {
    if (player == nullptr)
        return ExecutionStatus::NoPlayer;

    if (index >= path.size()) {
        clearInput(player);
        return ExecutionStatus::Arrived;
    }

    const auto feet = player->getFeetPosition();
    if (!progressInitialized) {
        lastProgressPosition = feet;
        progressInitialized = true;
    }

    while (index < path.size()) {
        const auto& node = path[index];
        const glm::vec3 target{static_cast<float>(node.pos.x) + 0.5f, static_cast<float>(node.pos.y), static_cast<float>(node.pos.z) + 0.5f};
        bool reached = horizontalDistance(feet, target) <= 0.30f && std::abs(feet.y - target.y) <= 0.70f;

        // A sprint jump often lands beyond the exact block center. Treat crossing
        // the landing plane while grounded as success so steering never reverses
        // and walks the player back toward the center of the landing block.
        if (!reached && node.movement == MovementType::Parkour && index > 0 && player->isOnGround() &&
            std::abs(feet.y - target.y) <= 0.70f) {
            const auto& sourceNode = path[index - 1];
            const glm::vec2 source{static_cast<float>(sourceNode.pos.x) + 0.5f, static_cast<float>(sourceNode.pos.z) + 0.5f};
            const glm::vec2 destination{target.x, target.z};
            const glm::vec2 jump = destination - source;
            const float lengthSquared = glm::dot(jump, jump);
            if (lengthSquared > 0.001f) {
                const glm::vec2 playerOffset{feet.x - source.x, feet.z - source.y};
                const float progress = glm::dot(playerOffset, jump) / lengthSquared;
                const glm::vec2 closest = source + jump * std::clamp(progress, 0.f, 1.f);
                const float lateral = glm::length(glm::vec2{feet.x, feet.z} - closest);
                reached = progress >= 0.88f && lateral <= 0.85f;
            }
        }

        if (!reached)
            break;
        ++index;
    }

    if (index >= path.size()) {
        clearInput(player);
        return ExecutionStatus::Arrived;
    }

    if (horizontalDistance(feet, lastProgressPosition) > 0.15f || std::abs(feet.y - lastProgressPosition.y) > 0.35f) {
        lastProgressPosition = feet;
        ticksWithoutProgress = 0;
    } else if (++ticksWithoutProgress > 50) {
        clearInput(player);
        return ExecutionStatus::Stuck;
    }

    const auto& node = path[index];
    const bool isParkour = node.movement == MovementType::Parkour;
    if (isParkour) {
        if (activeParkourIndex != index) {
            resetParkourState();
            activeParkourIndex = index;
        }
        if (!player->isOnGround())
            parkourWasAirborne = true;
        // If the launch became airborne but this same edge is still active on
        // the next grounded tick, the destination was missed. Stop immediately
        // and let the controller replan instead of walking off another edge.
        if (parkourJumpIssued && parkourWasAirborne && player->isOnGround()) {
            clearInput(player);
            return ExecutionStatus::Stuck;
        }
        if (parkourJumpIssued && !parkourWasAirborne && ++parkourLaunchTicks > 10) {
            clearInput(player);
            return ExecutionStatus::Stuck;
        }
    } else {
        resetParkourState();
    }

    const glm::vec3 target{static_cast<float>(node.pos.x) + 0.5f, static_cast<float>(node.pos.y), static_cast<float>(node.pos.z) + 0.5f};
    glm::vec2 direction{target.x - feet.x, target.z - feet.z};
    glm::vec2 facingDirection = direction;
    int parkourDistance = 0;
    bool parkourAscend = false;
    bool parkourReady = !isParkour;
    float parkourAlong = 0.f;
    float parkourProgress = 0.f;
    if (isParkour && index > 0) {
        const auto& source = path[index - 1].pos;
        const glm::vec2 sourceCenter{static_cast<float>(source.x) + 0.5f, static_cast<float>(source.z) + 0.5f};
        glm::vec2 jumpDirection{static_cast<float>(node.pos.x - source.x), static_cast<float>(node.pos.z - source.z)};
        parkourDistance = std::abs(node.pos.x - source.x) + std::abs(node.pos.z - source.z);
        parkourAscend = node.pos.y > source.y;

        const float jumpLength = glm::length(jumpDirection);
        if (jumpLength > 0.001f) {
            jumpDirection /= jumpLength;
            facingDirection = jumpDirection;
            const glm::vec2 fromSource{feet.x - sourceCenter.x, feet.z - sourceCenter.y};
            parkourAlong = glm::dot(fromSource, jumpDirection);
            parkourProgress = parkourAlong / jumpLength;
            const float lateral = glm::length(fromSource - jumpDirection * parkourAlong);
            // Flow-style action corridor: only launch while still centered on
            // the source block and aimed down the actual parkour segment.
            parkourReady = player->isOnGround() && lateral <= 0.45f && parkourAlong >= -0.65f && parkourAlong <= 0.55f;
            direction = (parkourReady || parkourJumpIssued) ? jumpDirection : sourceCenter - glm::vec2{feet.x, feet.z};
        }
    }
    const float distance = glm::length(direction);
    if (distance > 0.001f)
        direction /= distance;
    const float facingDistance = glm::length(facingDirection);
    if (facingDistance > 0.001f)
        facingDirection /= facingDistance;

    // Keep the actor facing the rendered path for the entire route. Movement is
    // resolved against this same yaw below, so W is genuinely forward and the
    // native sprint state no longer drops when the user's camera pointed away.
    if (facingDistance > 0.001f) {
        auto rotation = player->getRotation();
        const float desiredYaw = std::atan2(-facingDirection.x, facingDirection.y) * 180.f / std::numbers::pi_v<float>;
        rotation.y = desiredYaw;
        player->setRotation(rotation);
    }

    const float yaw = player->getRotation().y * std::numbers::pi_v<float> / 180.f;
    const glm::vec2 forward{-std::sin(yaw), std::cos(yaw)};
    const glm::vec2 right{-forward.y, forward.x};
    const float coastThreshold = parkourDistance >= 4 ? 0.82f : (parkourDistance == 3 ? 0.78f : 0.72f);
    const bool parkourCoasting = isParkour && parkourJumpIssued && !player->isOnGround() &&
        parkourProgress >= coastThreshold;
    // Reverse air input slightly after crossing the braking point. Releasing W
    // alone preserves too much Bedrock momentum and intermittently overshoots.
    const float forwardAmount = parkourCoasting ? -0.35f : std::clamp(glm::dot(forward, direction), -1.f, 1.f);
    const float leftAmount = parkourCoasting ? 0.f : std::clamp(-glm::dot(right, direction), -1.f, 1.f);
    const glm::vec2 localMovement{leftAmount, forwardAmount};

    const bool parkourNeedsSprint = isParkour && (parkourDistance >= 4 || parkourAscend);
    const float takeoffDistance = parkourDistance >= 4 ? 0.18f : (parkourDistance == 3 ? 0.30f : 0.f);
    const bool sprintReady = !parkourNeedsSprint || parkourSprintPrimed;
    const bool requestParkourJump = isParkour && parkourReady && !parkourJumpIssued && sprintReady &&
        parkourAlong >= takeoffDistance && forwardAmount > 0.98f;

    if (const auto input = player->tryGet<MoveInputComponent>()) {
        if (!movementModeCaptured) {
            previousCameraRelativeMovement = input->isCameraRelativeMovementEnabled;
            previousRotationControlledByMovement = input->isRotControlledByMoveDirection;
            movementModeCaptured = true;
        }
        // Use the actor/path yaw for all movement. Leaving this camera-relative
        // is what made the bot strafe or walk backward and lose sprint.
        input->isCameraRelativeMovementEnabled = false;
        input->isRotControlledByMoveDirection = true;

        input->move = localMovement;
        input->inputState.analogMoveVector = localMovement;
        input->rawInputState.analogMoveVector = localMovement;

        constexpr float digitalThreshold = 0.35f;
        input->inputState.up = forwardAmount > digitalThreshold;
        input->inputState.down = forwardAmount < -digitalThreshold;
        input->inputState.left = leftAmount > digitalThreshold;
        input->inputState.right = leftAmount < -digitalThreshold;
        input->rawInputState.up = input->inputState.up;
        input->rawInputState.down = input->inputState.down;
        input->rawInputState.left = input->inputState.left;
        input->rawInputState.right = input->inputState.right;

        // Java Baritone only forces sprint for the maximum-distance or ascending
        // parkour variants. Sprinting on short gaps causes Bedrock to overshoot.
        const bool shouldSprint = options.sprint && forwardAmount > (isParkour ? 0.55f : 0.8f) &&
            (!isParkour || parkourNeedsSprint) && !parkourCoasting;
        input->inputState.sprintDown = shouldSprint;
        input->rawInputState.sprintDown = shouldSprint;
        input->sprinting = shouldSprint;
        // Let Bedrock's normal movement systems consume the inputs. Locking
        // this component and writing velocity directly causes server lagbacks.
        input->moveInputStateLocked = false;

        if (isParkour && parkourReady && parkourNeedsSprint && shouldSprint)
            parkourSprintPrimed = true;
    }

    const bool shouldJump = (node.movement == MovementType::Ascend && target.y > feet.y + 0.2f) ||
        requestParkourJump;
    const bool shouldSwimUp = node.movement == MovementType::Swim && target.y > feet.y + 0.15f;
    const bool shouldSwimDown = node.movement == MovementType::Swim && target.y < feet.y - 0.15f;
    // Pulse jump while grounded instead of holding it throughout the flight.
    // This also prevents an immediate second jump on the landing tick.
    const bool holdJump = (shouldJump && player->isOnGround()) || shouldSwimUp;
    if (holdJump)
        player->jumpFromGround();
    if (requestParkourJump) {
        parkourJumpIssued = true;
        parkourLaunchTicks = 0;
    }
    if (const auto input = player->tryGet<MoveInputComponent>()) {
        input->jumping = holdJump;
        input->inputState.jumpDown = holdJump;
        input->inputState.jumpInputCurrentlyDown = holdJump;
        input->rawInputState.jumpDown = holdJump;
        input->rawInputState.jumpInputCurrentlyDown = holdJump;
    }

    if (const auto input = player->tryGet<MoveInputComponent>()) {
        input->sneaking = shouldSwimDown;
        input->wantDown = shouldSwimDown;
        input->inputState.sneakDown = shouldSwimDown;
        input->rawInputState.sneakDown = shouldSwimDown;
    }

    controlledMovement = true;
    return ExecutionStatus::Running;
}

void PathExecutor::stop(LocalPlayer* player) {
    clearInput(player);
    path.clear();
    index = 0;
    ticksWithoutProgress = 0;
    progressInitialized = false;
    resetParkourState();
}

void PathExecutor::suspend(LocalPlayer* player) {
    clearInput(player);
}

void PathExecutor::clearInput(LocalPlayer* player) {
    if (player == nullptr || !controlledMovement)
        return;

    if (const auto input = player->tryGet<MoveInputComponent>()) {
        if (movementModeCaptured) {
            input->isCameraRelativeMovementEnabled = previousCameraRelativeMovement;
            input->isRotControlledByMoveDirection = previousRotationControlledByMovement;
            movementModeCaptured = false;
        }
        input->move = {};
        input->inputState.analogMoveVector = {};
        input->rawInputState.analogMoveVector = {};
        input->inputState.up = false;
        input->inputState.down = false;
        input->inputState.left = false;
        input->inputState.right = false;
        input->inputState.sprintDown = false;
        input->inputState.jumpDown = false;
        input->inputState.jumpInputCurrentlyDown = false;
        input->rawInputState.up = false;
        input->rawInputState.down = false;
        input->rawInputState.left = false;
        input->rawInputState.right = false;
        input->rawInputState.sprintDown = false;
        input->rawInputState.jumpDown = false;
        input->rawInputState.jumpInputCurrentlyDown = false;
        input->inputState.sneakDown = false;
        input->rawInputState.sneakDown = false;
        input->sprinting = false;
        input->jumping = false;
        input->sneaking = false;
        input->wantDown = false;
        input->moveInputStateLocked = false;
    }

    controlledMovement = false;
}

void PathExecutor::resetParkourState() {
    activeParkourIndex = static_cast<std::size_t>(-1);
    parkourJumpIssued = false;
    parkourWasAirborne = false;
    parkourSprintPrimed = false;
    parkourLaunchTicks = 0;
}

std::size_t PathExecutor::getCurrentIndex() const { return index; }

const std::vector<PathNode>& PathExecutor::getPath() const { return path; }

} // namespace baritone
