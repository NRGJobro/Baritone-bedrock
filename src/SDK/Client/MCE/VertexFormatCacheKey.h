#pragma once

#include "VertexFormat.h"

namespace mce {
    struct VertexFormatCacheKey {
        VertexFormat vertexFormat;
        uint32_t attributeListIndex;
    };
}
