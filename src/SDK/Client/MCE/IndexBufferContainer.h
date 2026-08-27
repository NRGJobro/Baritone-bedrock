#pragma once

#include "../dragon/ResolvedIndexBufferResource.h"
#include "Buffer.h"
#include "ClientResourcePointer.h"

namespace mce {
    class IndexBufferContainer {
    public:
        ClientResourcePointer<std::variant<std::monostate, Buffer, ClientResourcePointer<dragon::mesh::ResolvedIndexBufferResource>>> indexBuffer;
        uint32_t indexCount;
        uint32_t indexSize;
        int iteration;
        int pad;
    };
}
