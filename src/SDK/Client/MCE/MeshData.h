#pragma once

#include "PrimitiveMode.h"
#include "VertexField.h"

namespace mce {
    class MeshData {
    public:
        PrimitiveMode mode;
        bool meshNotFree;
        std::vector<glm::vec3> positions;
        std::vector<glm::vec4> normals;
        std::vector<uint32_t> tangents;
        std::vector<uint32_t> indices;
        std::vector<uint32_t> colors;
        std::vector<uint16_t> boneId0s;
        std::vector<glm::vec2> textureUVs[3];
        std::vector<uint16_t> pbrTextureIndices;
        std::vector<uint32_t> mersList;
        std::vector<uint8_t> geoType;
        size_t unk1;
        size_t unk2;
        size_t unk3;
        size_t unk4;
        size_t unk5;
        bool fieldEnabled[15];

        MeshData& operator=(const MeshData& other);

        void clear();
        void enableField(VertexField vertexField);
        [[nodiscard]] bool hasField(VertexField vertexField) const;
        void reserveVertices(int maxVertices);
    };

    static_assert(offsetof(MeshData, geoType) == 0x110,
        "MeshData::geoType no longer matches the Minecraft 1.26.52 ABI");
    static_assert(offsetof(MeshData, fieldEnabled) == 0x150,
        "MeshData::fieldEnabled no longer matches the Minecraft 1.26.52 ABI");
    static_assert(sizeof(MeshData) == 0x160,
        "MeshData size no longer matches the Minecraft 1.26.52 ABI");
}
