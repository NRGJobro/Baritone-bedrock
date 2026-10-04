#pragma once

namespace mce {
    constexpr size_t VertexFormatFieldOffsetCount = 15;

    struct VertexFormat {
        uint16_t fieldMask{};
        uint16_t _fieldOffset[VertexFormatFieldOffsetCount];
        uint16_t vertexSize{};
        bool allowHalfFloats = true;

        VertexFormat() {
            memset(_fieldOffset, -1, VertexFormatFieldOffsetCount * sizeof(uint16_t));
        }
    };

    static_assert(sizeof(VertexFormat) == 0x24);
}
