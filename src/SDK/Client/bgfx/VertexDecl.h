#pragma once

namespace bgfx {
    struct VertexDecl {
        uint32_t hash;
        uint16_t stride;
        uint16_t offset[18];
        uint16_t attributes[18];
    };
}
