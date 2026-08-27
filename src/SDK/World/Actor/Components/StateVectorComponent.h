#pragma once

#include "IEntityComponent.h"

struct StateVectorComponent : IEntityComponent {
    glm::vec3 pos;
    glm::vec3 posPrev;
    glm::vec3 velocity;
};
