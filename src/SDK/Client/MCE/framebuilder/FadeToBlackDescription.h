#pragma once

#include "../Mesh.h"

namespace mce::framebuilder {
    struct FadeToBlackDescription {
        Mesh& mesh;
        float alpha;
        glm::mat4x4 worldTransform;
        uint16_t viewId;
    };
}
