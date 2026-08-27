#pragma once

#include "FatalError.h"
#include "FrameBufferD3D12.h"
#include "FrameBufferHandle.h"
#include "Resolution.h"

namespace bgfx::d3d12 {
    class RendererContextD3D12 {
    public:
        IDXGISwapChain* getSwapChain();
        int64_t& getPresentElapsed();
        uint16_t getNumWindows();
        FrameBufferHandle* getWindows();
        Resolution& getResolution();
        FatalError& getFatalError();
        bool& getUnknownFlag();
        FrameBufferD3D12* getFrameBuffers();
    };
}
