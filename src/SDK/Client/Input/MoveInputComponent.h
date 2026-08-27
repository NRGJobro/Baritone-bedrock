#pragma once

#include "MoveInputState.h"
#include "../../World/Actor/Components/IEntityComponent.h"

struct MoveInputComponent : IEntityComponent {
    MoveInputState inputState{};
    MoveInputState rawInputState{};
    int8_t holdAutoJumpInWaterTicks{};
    glm::vec2 move{};
    glm::vec2 lookDelta{};
    glm::vec2 interactDirection{};
    glm::vec3 displacement{};
    glm::vec3 displacementDelta{};
    glm::vec3 cameraOrientation{};
    bool sneaking : 1;
    bool sprinting : 1;
    bool wantUp : 1;
    bool wantDown : 1;
    bool jumping : 1;
    bool autoJumpingInWater : 1;
    bool moveInputStateLocked : 1;
    bool persistSneak : 1;
    bool autoJumpEnabled : 1;
    bool isCameraRelativeMovementEnabled : 1;
    bool isRotControlledByMoveDirection : 1;
    std::array<bool, 2> isPaddling{};
};
