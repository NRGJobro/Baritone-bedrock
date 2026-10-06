#pragma once

#include "ClientResourcePointer.h"

namespace mce {
    class IndexBufferContainer {
    public:
        ClientResourcePointer<void> indexBuffer;
        std::uint32_t indexCount;
        std::uint32_t indexSize;
        int iteration;
        int pad;
    };
}
