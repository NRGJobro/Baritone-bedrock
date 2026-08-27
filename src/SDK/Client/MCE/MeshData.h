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
        bool fieldEnabled[14];

        MeshData& operator=(const MeshData& other);

        void clear();
        void enableField(VertexField vertexField);
        [[nodiscard]] bool hasField(VertexField vertexField) const;
        void reserveVertices(int maxVertices);
    };
}
