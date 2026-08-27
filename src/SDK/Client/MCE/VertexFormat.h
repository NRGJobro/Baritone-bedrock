#pragma once

namespace mce {
    struct VertexFormat {
        uint16_t fieldMask{};
        uint16_t _fieldOffset[14];
        uint16_t vertexSize{};
        bool allowHalfFloats = true;

        VertexFormat() {
            memset(_fieldOffset, -1, 14 * sizeof(uint16_t));
        }
    };
}
