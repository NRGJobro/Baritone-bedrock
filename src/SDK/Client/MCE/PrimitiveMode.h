#pragma once

namespace mce {
    enum class PrimitiveMode : uint8_t {
        None,
        QuadList,
        TriangleList,
        TriangleStrip,
        LineList,
        LineStrip
    };
}
