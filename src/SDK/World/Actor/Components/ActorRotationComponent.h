#pragma once

#include "IEntityComponent.h"

struct ActorRotationComponent : IEntityComponent {
    glm::vec2 rotation;
    glm::vec2 previousRotation;
};
