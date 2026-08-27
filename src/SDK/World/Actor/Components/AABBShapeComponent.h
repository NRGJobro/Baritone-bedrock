#pragma once

#include "../../../Util/AABB.h"
#include "IEntityComponent.h"

struct AABBShapeComponent : IEntityComponent {
    AABB aabb;
    glm::vec2 size;
};
