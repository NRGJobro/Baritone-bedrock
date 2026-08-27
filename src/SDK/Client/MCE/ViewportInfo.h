#pragma once

namespace mce {
    struct ViewportInfo {
        glm::vec2 size;
        glm::vec2 offset;
        float minDepth;
        float maxDepth;
    };
}
