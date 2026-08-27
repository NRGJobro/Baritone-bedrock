#include "FrameBufferD3D11.h"

HRESULT bgfx::d3d11::FrameBufferD3D11::present(const uint32_t syncInterval, const uint32_t flags) {
    if (needPresent) {
        const auto hr = swapChain->Present(syncInterval, flags);
        needPresent = false;
        return hr;
    }

    return S_OK;
}
