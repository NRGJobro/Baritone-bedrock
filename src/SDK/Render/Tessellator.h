#pragma once

#include "../Client/MCE/Color.h"
#include "../Client/MCE/Mesh.h"
#include "../Client/MCE/MeshData.h"

struct TessellatorQuadInfo {
    std::uint8_t facing;
    bool twoFace;
    glm::vec3 centroid;
};

class Tessellator {
    bool isFormatFixed;
    mce::MeshData meshData;
    bool hasNormals;
    uint64_t nextReserve;
    std::optional<glm::vec4> nextNormal;
    std::optional<glm::vec2> nextUV[3];
    std::optional<uint32_t> nextColor;
    std::optional<uint16_t> nextBoneId;
    std::optional<uint16_t> nextPBRTextureIdx;
    std::optional<uint32_t> nextMers;
    bool indexPhase;
    glm::vec3 postTransformationOffset;
    glm::vec3 postTransformationScale;
    uint8_t quadFacing;
    bool quadTwoSided;
    std::vector<TessellatorQuadInfo> quadInfoList;
    glm::vec3 faceCenterAccumulator;
    int currQuadIndex;
    bool applyTransform;
    glm::mat4x4 transformMatrix;
    bool noColor;
    bool overriden;
    bool forceTessellateIntercept;
    std::function<void __cdecl(Tessellator*, mce::MaterialPtr*, const mce::TexturePtr&)> interceptTessellator;
    int count;
    int maxFaces;
    bool tessellating;
    bool buildFaceData;
    std::unique_ptr<mce::Mesh> preGeneratedMesh;
    std::weak_ptr<mce::BufferResourceService> bufferResourceService;

public:
    enum class UploadMode : int {
        Buffered,
        Manual,
        Never
    };

    void begin(mce::PrimitiveMode mode = mce::PrimitiveMode::TriangleList, int maxVertices = 0, bool buildFaceData = false);

    void vertex(float x, float y, float z = 0.f);
    void vertex(const glm::vec3& pos);
    void vertexChroma(float x, float y, float alpha = 1.f);

    void vertexUV(float x, float y, float z, float u, float v);

    void color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void color(float r, float g, float b, float a = 1.f);
    void color(const mce::Color &color);
    void color(const mce::Color &color, float alpha);

    void end(mce::Mesh& mesh);
    void clear();

    [[nodiscard]] size_t getFaces() const;
    [[nodiscard]] size_t getVertices() const;

    [[nodiscard]] bool isTessellating() const;

    static int getFaceVertexCountForMode(mce::PrimitiveMode mode);
};
