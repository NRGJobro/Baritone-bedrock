#pragma once

#include "../Mesh.h"
#include "../../dragon/res/ServerTexture.h"

namespace mce::framebuilder {
    struct FullscreenEffectDescription {
        Mesh& mesh;
        dragon::res::ServerTexture texture;
        glm::mat4x4 worldMatrix;
        glm::vec3 cameraPosition;
        Color shaderColor;
        MaterialPtr material;
        uint16_t viewId;
        dragon::RenderMetadata renderMetadata;
    };
}
