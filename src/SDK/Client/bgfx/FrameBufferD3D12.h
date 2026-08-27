#pragma once

#include "Attachment.h"

namespace bgfx::d3d12 {
    class FrameBufferD3D12 {
    public:
        TextureHandle texture[8];
        TextureHandle depth;
        IDXGISwapChain* swapChain;
        void* nwh;
        uint32_t width;
        uint32_t height;
        uint16_t denseIdx;
        uint8_t num;
        uint8_t numTh;
        Attachment attachment[8];
        bool needPresent;

        HRESULT present(uint32_t syncInterval, uint32_t flags);
    };
}
