#include "Mesh.h"

#include "../../../Memory/Sig/SignatureManager.h"
#include "../../../Utils/Utils.h"

mce::Mesh::~Mesh() {
    reset();
}

void mce::Mesh::reset() {
    if (this->vertexLayout.resourcePointerBlock != nullptr) {
        this->vertexLayout.decRef();
        this->vertexLayout.resourcePointerBlock = nullptr;
    }

    if (this->vertexBuffer.resourcePointerBlock != nullptr) {
        this->vertexBuffer.decRef();
        this->vertexBuffer.resourcePointerBlock = nullptr;
    }

    if (this->indexBuffer.resourcePointerBlock != nullptr) {
        this->indexBuffer.decRef();
        this->indexBuffer.resourcePointerBlock = nullptr;
    }

    this->vertexCount.reset();

    this->primitiveMode = PrimitiveMode::None;

    this->meshData.clear();

    this->layoutFormat.fieldMask = 0;
    memset(this->layoutFormat._fieldOffset, 0, VertexFormatFieldOffsetCount * sizeof(uint16_t));
    this->layoutFormat.allowHalfFloats = true;

    this->bufferFormat.fieldMask = 0;
    memset(this->bufferFormat._fieldOffset, 0, VertexFormatFieldOffsetCount * sizeof(uint16_t));
    this->bufferFormat.allowHalfFloats = true;

    this->unkFormat.fieldMask = 0;
    memset(this->unkFormat._fieldOffset, 0, VertexFormatFieldOffsetCount * sizeof(uint16_t));
    this->unkFormat.allowHalfFloats = true;
}

bool mce::Mesh::areVertexFormatsValid() const {
    constexpr uint16_t val = 0xFFFF;

    if (this->layoutFormat.fieldMask == 0 && this->layoutFormat.vertexSize == 0 &&
        memcmp(this->layoutFormat._fieldOffset, &val, VertexFormatFieldOffsetCount * sizeof(uint16_t)) == 0)
        return false;

    if (this->bufferFormat.fieldMask == 0 && this->bufferFormat.vertexSize == 0 &&
        memcmp(this->bufferFormat._fieldOffset, &val, VertexFormatFieldOffsetCount * sizeof(uint16_t)) == 0)
        return false;

    return true;
}

bool mce::Mesh::isValid() const {
    if (this->meshData.positions.empty()) {
        if (!this->areVertexFormatsValid() || !this->vertexCount.has_value() || this->vertexCount.value() == 0)
            return false;
    }

    return true;
}

void mce::Mesh::renderMesh(MeshContext* meshContext, MaterialPtr* material) const {
    using CaptureDescription = std::variant<std::monostate, UIActorOffscreenCaptureDescription, UIThumbnailMeshOffscreenCaptureDescription, UIMeshOffscreenCaptureDescription, UIStructureVolumeOffscreenCaptureDescription>;
    using func_t = void(*)(const Mesh*, MeshContext*, MaterialPtr*, const StaticVector<std::variant<std::monostate, TexturePtr, ClientTexture>, 8>&, uint32_t, uint32_t, const CaptureDescription&, void*, const std::optional<dragon::RenderMetadata>&);
    static auto func = Utils::getFromOffset<func_t>(GET_SIG("mce::Mesh::_renderMesh"), 1);
    func(this, meshContext, material, {}, 0, 0, UIMeshOffscreenCaptureDescription(), nullptr, {});
}

void mce::Mesh::renderMeshFull(MeshContext* meshContext, MaterialPtr* material, StaticVector<std::variant<std::monostate, TexturePtr, ClientTexture>, 8> textures,
    uint32_t startOffset, uint32_t count,
    const std::variant<std::monostate, UIActorOffscreenCaptureDescription, UIThumbnailMeshOffscreenCaptureDescription, UIMeshOffscreenCaptureDescription, UIStructureVolumeOffscreenCaptureDescription>& variant,
    void* overrideIndexBuffer, const std::optional<dragon::RenderMetadata>& metadata) const {
    static auto sig = Utils::getFromOffset<uintptr_t>(GET_SIG("mce::Mesh::_renderMesh"), 1);
    static auto func = *(decltype(&Mesh::renderMeshFull)*)&sig;
    (this->*func)(meshContext, material, textures, startOffset, count, variant, overrideIndexBuffer, metadata);
}
