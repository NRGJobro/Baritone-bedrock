#pragma once

namespace bgfx {
    struct SwapChainDesc {
        uint32_t width;
        uint32_t height;
        DXGI_FORMAT format;
        bool stereo;
        DXGI_SAMPLE_DESC sampleDesc;
        DXGI_USAGE bufferUsage;
        uint32_t bufferCount;
        DXGI_SCALING scaling;
        DXGI_SWAP_EFFECT swapEffect;
        DXGI_ALPHA_MODE alphaMode;
        uint32_t flags;
        void* nwh;
        void* ndt;
        bool windowed;
    };
}
