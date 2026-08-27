#pragma once

#include "IEntityComponent.h"

struct RenderPositionComponent : IEntityComponent {
    glm::vec3 pos;
};
