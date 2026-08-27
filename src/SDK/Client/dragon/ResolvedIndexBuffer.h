#pragma once

#include "../bgfx/DynamicIndexBufferHandle.h"
#include "../bgfx/IndexBufferHandle.h"
#include "IndexBufferView.h"
#include "IndexSize.h"

namespace dragon::mesh {
    class ResolvedIndexBuffer {
        std::variant<bgfx::IndexBufferHandle, bgfx::DynamicIndexBufferHandle> resource;
        uint32_t offset;
        uint32_t count;
        IndexSize indexSize;
        std::unique_ptr<IndexBufferView> storageView;
    };
}
