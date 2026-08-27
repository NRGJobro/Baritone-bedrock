#pragma once

#include "ComparisonFunc.h"
#include "StencilOp.h"

namespace mce {
    struct StencilFaceDescription {
        ComparisonFunc stencilFunc;
        StencilOp stencilDepthFailOp;
        StencilOp stencilPassOp;
        StencilOp stencilFailOp;
    };
}
