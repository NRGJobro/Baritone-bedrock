#include "FrameBufferD3D12.h"

HRESULT bgfx::d3d12::FrameBufferD3D12::present(const uint32_t syncInterval, const uint32_t flags) {
    if (needPresent) {
        const auto hr = swapChain->Present(syncInterval, flags);
        needPresent = false;
        return hr;
    }

    return S_OK;
}
