#include "MeshData.h"

mce::MeshData& mce::MeshData::operator=(const MeshData& other) {
    this->mode = other.mode;
    this->positions = other.positions;
    /*this->normals = other.normals;
    this->tangents = other.tangents;
    this->indices = other.indices;*/
    this->colors = other.colors;
    //this->boneId0s = other.boneId0s;

    /*for (int i = 0; i < 3; i++) {
        this->textureUVs[i] = other.textureUVs[i];
    }*/

    this->textureUVs[0] = other.textureUVs[0];

    /*this->pbrTextureIndices = other.pbrTextureIndices;
    this->mersList = other.mersList;*/
    memcpy(this->fieldEnabled, other.fieldEnabled, 14);

    return *this;
}

void mce::MeshData::clear() {
    this->mode = PrimitiveMode::None;

    this->positions.clear();
    //this->normals.clear();
    //this->tangents.clear();
    //this->indices.clear();
    this->colors.clear();
    //this->boneId0s.clear();

    /*for (auto& vec : this->textureUVs) {
        vec.clear();
    }*/

    this->textureUVs[0].clear();

    //this->pbrTextureIndices.clear();
    //this->mersList.clear();

    memset(this->fieldEnabled, 0, 14);
}

void mce::MeshData::enableField(VertexField vertexField) {
    const int index = *reinterpret_cast<int*>(&vertexField) & 0xFFFF;

    /*if (index < 0 || index > 14)
        return;*/

    this->fieldEnabled[index] = true;
}

bool mce::MeshData::hasField(VertexField vertexField) const {
    const int index = *reinterpret_cast<int*>(&vertexField);

    /*if (index < 0 || index > 14)
        return;*/

    return this->fieldEnabled[index];
}

void mce::MeshData::reserveVertices(const int maxVertices) {
    this->positions.reserve(maxVertices);
    //this->normals.reserve(maxVertices);
    //this->tangents.reserve(maxVertices);
    this->colors.reserve(maxVertices);
    //this->boneId0s.reserve(maxVertices);
    //this->pbrTextureIndices.reserve(maxVertices);

    /*for (auto& vec : this->textureUVs) {
        vec.reserve(maxVertices);
    }*/

    this->textureUVs[0].reserve(maxVertices);
}
