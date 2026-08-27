#pragma once

#include "CullMode.h"
#include "FillMode.h"

namespace mce {
    struct RasterizerStateDescription {
        float depthBias;
        float slopeScaledDepthBias;
        CullMode cullMode;
        FillMode fillMode;
    };
}
