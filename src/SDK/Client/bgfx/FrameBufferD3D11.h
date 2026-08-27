#pragma once

#include "Attachment.h"

namespace bgfx::d3d11 {
    class FrameBufferD3D11 {
    public:
        void* renderTargetView[7];
        void* shaderResourceView[7];
        void* depthStencilView;
        IDXGISwapChain* swapChain;
        uint32_t width;
        uint32_t height;
        Attachment attachment[8];
        uint16_t denseIdx;
        uint8_t num;
        uint8_t numTh;
        bool needPresent;

        HRESULT present(uint32_t syncInterval, uint32_t flags);
    };
}
