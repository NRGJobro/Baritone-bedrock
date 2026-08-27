#include "RendererContextD3D12.h"

IDXGISwapChain* bgfx::d3d12::RendererContextD3D12::getSwapChain() {
    return hat::member_at<IDXGISwapChain*>(this, 0x308);
}

int64_t& bgfx::d3d12::RendererContextD3D12::getPresentElapsed() {
    return hat::member_at<int64_t>(this, 0x318);
}

uint16_t bgfx::d3d12::RendererContextD3D12::getNumWindows() {
    return hat::member_at<uint16_t>(this, 0x320);
}

bgfx::FrameBufferHandle* bgfx::d3d12::RendererContextD3D12::getWindows() {
    return hat::member_at<FrameBufferHandle*>(this, 0x324);
}

bgfx::Resolution& bgfx::d3d12::RendererContextD3D12::getResolution() {
    return hat::member_at<Resolution>(this, 0x19FA0);
}

bgfx::FatalError& bgfx::d3d12::RendererContextD3D12::getFatalError() {
    return hat::member_at<FatalError>(this, 0x19FB0);
}

bool& bgfx::d3d12::RendererContextD3D12::getUnknownFlag() {
    return hat::member_at<bool>(this, 0x1A005);
}

bgfx::d3d12::FrameBufferD3D12* bgfx::d3d12::RendererContextD3D12::getFrameBuffers() {
    return hat::member_at<FrameBufferD3D12*>(this, 0xC976C0);
}
