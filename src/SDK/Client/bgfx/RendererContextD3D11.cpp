#include "RendererContextD3D11.h"

IDXGISwapChain* bgfx::d3d11::RendererContextD3D11::getSwapChain() {
    return hat::member_at<IDXGISwapChain*>(this, 0x228);
}

bool& bgfx::d3d11::RendererContextD3D11::getNeedPresent() {
    return hat::member_at<bool>(this, 0x238);
}

bgfx::FatalError& bgfx::d3d11::RendererContextD3D11::getFatalError() {
    return hat::member_at<FatalError>(this, 0x23C);
}

uint16_t bgfx::d3d11::RendererContextD3D11::getNumWindows() {
    return hat::member_at<uint16_t>(this, 0x244);
}

bgfx::FrameBufferHandle* bgfx::d3d11::RendererContextD3D11::getWindows() {
    return hat::member_at<FrameBufferHandle*>(this, 0x246);
}

ID3D11DeviceContext* bgfx::d3d11::RendererContextD3D11::getDeviceContext() {
    return hat::member_at<ID3D11DeviceContext*>(this, 0x350);
}

bgfx::Resolution& bgfx::d3d11::RendererContextD3D11::getResolution() {
    return hat::member_at<Resolution>(this, 0xD3DC);
}

bgfx::d3d11::FrameBufferD3D11* bgfx::d3d11::RendererContextD3D11::getFrameBuffers() {
    return hat::member_at<FrameBufferD3D11*>(this, 0x1FA968);
}
