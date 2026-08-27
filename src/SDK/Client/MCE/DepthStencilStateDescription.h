#pragma once

#include "DepthWriteMask.h"
#include "StencilFaceDescription.h"

namespace mce {
    struct DepthStencilStateDescription {
        bool depthTestEnabled;
        bool stencilTestEnabled;
        ComparisonFunc depthFunc;
        StencilFaceDescription frontFace;
        StencilFaceDescription backFace;
        DepthWriteMask depthWriteMask;
        uint32_t stencilReadMask;
        uint32_t stencilWriteMask;
        uint8_t stencilRef;
        uint8_t originalStencilRef;
        bool overwroteStencilRef;
    };
}
