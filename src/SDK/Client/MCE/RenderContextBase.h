#pragma once

#include "RenderContextStateBase.h"
#include "RenderTargetState.h"
#include "VertexFormatCacheKey.h"

namespace mce {
    struct RenderContextBase {
        RenderContextStateBase currentState;
        VertexFormatCacheKey lastVertexFormat;
        std::array<void*, 3> lastPrograms;
        void* unk;
        void* frameBufferObject;
        void* renderDevice;
        RenderTargetState renderTargetState;
    };
}
