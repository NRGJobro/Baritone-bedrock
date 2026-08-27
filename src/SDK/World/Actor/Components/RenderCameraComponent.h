#pragma once

#include "../../../Render/Matrix/Matrix.h"
#include "../../../Util/HashedString.h"
#include "IEntityComponent.h"

namespace MinecraftCamera {
    struct RenderCameraComponent : IEntityComponent {
        HashedString id;
        glm::qua<float> quaternion;
        glm::vec3 position;
        float aspectRatio;
        float fieldOfView;
        float nearPlane;
        float farPlane;
        Matrix postViewTransform;
        Matrix savedProjection;
        Matrix savedModelView;
    };
}
