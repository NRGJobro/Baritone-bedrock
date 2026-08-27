#pragma once

#include "BlendStateDescription.h"
#include "DepthStencilStateDescription.h"
#include "RasterizerStateDescription.h"
#include "RenderTargetState.h"
#include "SamplerStateDescription.h"
#include "TextureBase.h"
#include "ViewportInfo.h"

namespace mce {
    struct RenderContextStateBase {
        BlendStateDescription blendStateDescription;
        DepthStencilStateDescription depthStencilStateDescription;
        RasterizerStateDescription rasterizerStateDescription;
        SamplerStateDescription samplerStateDescription[8];
        bool textureUnitActive[8];
        glm::vec4 currentScissor;
        ViewportInfo currentViewport;
        bool initializedSamplerState[8];
        std::array<const TextureBase*, 8> lastBoundTexture;
        RenderTargetState* renderTargetState;
    };
}
