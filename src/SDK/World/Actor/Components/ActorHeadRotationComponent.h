#pragma once

#include "IEntityComponent.h"

// ECS-owned Bedrock component. x is the current head yaw and y is its
// interpolation endpoint, matching the fields used by the actor renderer.
struct ActorHeadRotationComponent : IEntityComponent {
    glm::vec2 rotation;
};
