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
        if (horizontalDistance(feet, target) > 0.30f || std::abs(feet.y - target.y) > 0.70f)
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
    const glm::vec3 target{static_cast<float>(node.pos.x) + 0.5f, static_cast<float>(node.pos.y), static_cast<float>(node.pos.z) + 0.5f};
    glm::vec2 direction{target.x - feet.x, target.z - feet.z};
    const float distance = glm::length(direction);
    if (distance > 0.001f)
        direction /= distance;

    // Free-look steering: convert the desired world direction into the same
    // local left/right + backward/forward vector produced by normal controls.
    // This avoids fighting the camera/rotation component every simulation tick.
    const float yaw = player->getRotation().y * std::numbers::pi_v<float> / 180.f;
    const glm::vec2 forward{-std::sin(yaw), std::cos(yaw)};
    const glm::vec2 right{-forward.y, forward.x};
    const float forwardAmount = std::clamp(glm::dot(forward, direction), -1.f, 1.f);
    const float leftAmount = std::clamp(-glm::dot(right, direction), -1.f, 1.f);
    const glm::vec2 localMovement{leftAmount, forwardAmount};

    if (const auto input = player->tryGet<MoveInputComponent>()) {
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

        const bool shouldSprint = options.sprint && forwardAmount > 0.8f;
        input->inputState.sprintDown = shouldSprint;
        input->rawInputState.sprintDown = shouldSprint;
        input->sprinting = shouldSprint;
        // Let Bedrock's normal movement systems consume the inputs. Locking
        // this component and writing velocity directly causes server lagbacks.
        input->moveInputStateLocked = false;
    }

    const bool shouldJump = node.movement == MovementType::Ascend && target.y > feet.y + 0.2f;
    const bool shouldSwimUp = node.movement == MovementType::Swim && target.y > feet.y + 0.15f;
    const bool shouldSwimDown = node.movement == MovementType::Swim && target.y < feet.y - 0.15f;
    if ((shouldJump && player->isOnGround()) || shouldSwimUp) {
        player->jumpFromGround();
        if (const auto input = player->tryGet<MoveInputComponent>()) {
            input->jumping = true;
            input->inputState.jumpDown = true;
            input->inputState.jumpInputCurrentlyDown = true;
            input->rawInputState.jumpDown = true;
            input->rawInputState.jumpInputCurrentlyDown = true;
        }
    } else if (const auto input = player->tryGet<MoveInputComponent>()) {
        input->jumping = false;
        input->inputState.jumpDown = false;
        input->inputState.jumpInputCurrentlyDown = false;
        input->rawInputState.jumpDown = false;
        input->rawInputState.jumpInputCurrentlyDown = false;
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
}

void PathExecutor::suspend(LocalPlayer* player) {
    clearInput(player);
}

void PathExecutor::clearInput(LocalPlayer* player) {
    if (player == nullptr || !controlledMovement)
        return;

    if (const auto input = player->tryGet<MoveInputComponent>()) {
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

std::size_t PathExecutor::getCurrentIndex() const { return index; }

const std::vector<PathNode>& PathExecutor::getPath() const { return path; }

} // namespace baritone
