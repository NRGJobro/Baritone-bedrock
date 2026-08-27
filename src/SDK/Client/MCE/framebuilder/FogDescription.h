#pragma once

namespace mce::framebuilder {
    struct FogDescription {
        glm::vec4 color;
        glm::vec2 control;
        float renderDistance;
    };
}
