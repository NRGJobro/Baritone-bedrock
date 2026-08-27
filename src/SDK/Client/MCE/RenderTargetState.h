#pragma once

#include "PrimitiveMode.h"
#include "TextureFormat.h"

namespace mce {
    struct RenderTargetState {
        struct StencilOverride {
            uint8_t bits : 4;
            bool enabled;
        };

        StencilOverride highStencilBitsOverride;
        std::array<TextureFormat,8> renderTargetFormat;
        TextureFormat depthTextureFormat;
        uint32_t currentRenderTargetsBound;
        uint32_t currentRenderTargetMSAACount;
        PrimitiveMode lastPrimitiveMode;
        uint8_t lastStencilRef;
        uint8_t stencilRef;
    };
}
