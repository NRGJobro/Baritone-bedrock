#pragma once

#include "../../../Utils/StaticVector.h"
#include "BufferResourceService.h"
#include "IndexBufferContainer.h"
#include "MaterialPtr.h"
#include "MeshContext.h"
#include "MeshData.h"
#include "TexturePtr.h"

namespace mce {
    class Mesh : public IndexBufferContainer {
    public:
        std::variant<std::monostate, uint64_t, glm::vec3> cacheKey;
        bool temporary;
        PrimitiveMode primitiveMode;
        std::weak_ptr<BufferResourceService> bufferResourceService;
        MeshData meshData;
        ClientResourcePointer<void> vertexLayout;
        ClientResourcePointer<void> vertexBuffer;
        std::optional<uint32_t> vertexCount;
        VertexFormat layoutFormat;
        VertexFormat bufferFormat;
        VertexFormat unkFormat;
        std::vector<uint8_t> rawData;

        ~Mesh();

        void reset();

        bool areVertexFormatsValid() const;
        bool isValid() const;

        void renderMesh(MeshContext* meshContext, MaterialPtr* material) const;
    };
}
