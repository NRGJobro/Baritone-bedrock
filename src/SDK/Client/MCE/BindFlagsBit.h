#pragma once

namespace mce {
    enum class BindFlagsBit {
        NoBindFlags = 0x0,
        VertexBufferBit = 0x1,
        IndexBufferBit = 0x2,
        ConstantBufferBit = 0x4,
        ShaderResourceBit = 0x8,
        StreamOutputBit = 0x10,
        RenderTargetBit = 0x20,
        DepthStencilBit = 0x40,
        UnorderedAccessBit = 0x80
    };
}
