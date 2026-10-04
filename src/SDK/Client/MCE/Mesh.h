#pragma once

#include "../../../Utils/StaticVector.h"
#include "../../Render/Resources/UIActorOffscreenCaptureDescription.h"
#include "../../Render/Resources/UIThumbnailMeshOffscreenCaptureDescription.h"
#include "../../Render/Resources/UIMeshOffscreenCaptureDescription.h"
#include "../../Render/Resources/UIStructureVolumeOffscreenCaptureDescription.h"
#include "../dragon/RenderMetadata.h"
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
        ClientResourcePointer<std::variant<std::monostate, Buffer, ClientResourcePointer<dragon::mesh::ResolvedVertexBufferResource>>> vertexLayout;
        ClientResourcePointer<std::variant<std::monostate, Buffer, ClientResourcePointer<dragon::mesh::ResolvedVertexBufferResource>>> vertexBuffer;
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

    private:
        void renderMeshFull(MeshContext* meshContext, MaterialPtr* material, StaticVector<std::variant<std::monostate, TexturePtr, ClientTexture>, 8> textures,
            uint32_t startOffset, uint32_t count,
            const std::variant<std::monostate, UIActorOffscreenCaptureDescription, UIThumbnailMeshOffscreenCaptureDescription, UIMeshOffscreenCaptureDescription, UIStructureVolumeOffscreenCaptureDescription>& variant,
            void* overrideIndexBuffer, const std::optional<dragon::RenderMetadata>& metadata) const;
    };
}
