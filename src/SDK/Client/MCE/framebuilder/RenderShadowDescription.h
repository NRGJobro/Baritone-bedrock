#pragma once

#include "../../../Render/RenderObject/ActorShadowRenderObjectCollection.h"

namespace mce::framebuilder {
    struct RenderShadowDescription {
        ActorShadowRenderObjectCollection& actorShadowRenderObjectCollection;
        glm::mat4x4& worldMatrix;
        glm::vec3& cameraPosition;
        uint16_t viewId;
    };
}
