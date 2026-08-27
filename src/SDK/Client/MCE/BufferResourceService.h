#pragma once

#include "../dragon/ResolvedVertexBufferResource.h"
#include "Buffer.h"
#include "CheckedResourceService.h"
#include "ClientResourcePointer.h"

namespace mce {
    struct BufferResourceService : CheckedResourceService<std::variant<std::monostate, Buffer,
        ClientResourcePointer<dragon::mesh::ResolvedVertexBufferResource>>> { };
}
