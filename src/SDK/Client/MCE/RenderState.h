#pragma once

#ifndef BIT
#define BIT(x) (1 << x)
#endif

namespace mce {
    enum RenderState : uint16_t {
        None = 0,
        DisableDepthTest = BIT(0),
        Blending = BIT(1),
        DisableCulling = BIT(2),
        DisableColorWrite = BIT(3),
        DisableAlphaWrite = BIT(4),
        DisableDepthWrite = BIT(5),
        StencilWrite = BIT(6),
        InvertCulling = BIT(7),
        EnableStencilTest = BIT(8),
        EnableAlphaToCoverage = BIT(9),
        DisableRGBWrite = BIT(11),
        _count = DisableRGBWrite + 1
    };
}
