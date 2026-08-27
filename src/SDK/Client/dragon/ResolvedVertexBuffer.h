#pragma once

#include "../bgfx/DynamicVertexBufferHandle.h"
#include "../bgfx/VertexBufferHandle.h"
#include "../bgfx/VertexDecl.h"
#include "../bgfx/VertexDeclHandle.h"
#include "VertexBufferView.h"

namespace dragon::mesh {
    class ResolvedVertexBuffer {
    public:
        std::variant<bgfx::VertexBufferHandle, bgfx::DynamicVertexBufferHandle> resource;
        uint32_t offset;
        uint32_t count;
        bgfx::VertexDeclHandle vertexDeclHandle;
        bgfx::VertexDecl vertexDecl;
        std::unique_ptr<VertexBufferView> storageView;
    };
}
