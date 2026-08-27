#pragma once

namespace mce {
    enum StencilOp : uint8_t {
        StencilOpKeep = 0x1,
        StencilOpZero = 0x2,
        StencilOpReplace = 0x3,
        StencilOpIncr = 0x7,
        StencilOpDecr = 0x8
    };
}
