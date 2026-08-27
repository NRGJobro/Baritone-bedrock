#pragma once

#include "BufferBase.h"

namespace mce {
    class BufferNull : public BufferBase {
    public:
        uint64_t dataSize;
    };
}
