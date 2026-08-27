#pragma once

#include "FogDescription.h"
#include "../Mesh.h"
#include "../../DFC/ViewSetId.h"
#include "../../dragon/res/ServerTexture.h"

namespace mce::framebuilder {
    struct RenderFlameBillboardDescription {
        Mesh& mesh;
        glm::mat4x4 worldMatrix;
        dragon::res::ServerTexture texture;
        bool isUIPass;
        glm::vec2 uvOffset;
        FogDescription fog;
        float farChunkDistance;
        DFC::ViewSetId viewSetId;
    };
}
