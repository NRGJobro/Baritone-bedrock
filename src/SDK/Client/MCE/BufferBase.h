#pragma once

#include "BufferType.h"

namespace mce {
    class BufferBase {
        BufferType bufferType;
        uint32_t stride;
        uint32_t count;
        uint32_t internalSize;
        uint32_t bufferOffset;
    };
}
