#pragma once

#include "FatalError.h"
#include "FrameBufferD3D11.h"
#include "FrameBufferHandle.h"
#include "Resolution.h"

namespace bgfx::d3d11 {
    class RendererContextD3D11 {
    public:
        IDXGISwapChain* getSwapChain();
        bool& getNeedPresent();
        FatalError& getFatalError();
        uint16_t getNumWindows();
        FrameBufferHandle* getWindows();
        ID3D11DeviceContext* getDeviceContext();
        Resolution& getResolution();
        FrameBufferD3D11* getFrameBuffers();
    };
}
