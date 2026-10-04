#include "MeshData.h"

mce::MeshData& mce::MeshData::operator=(const MeshData& other) {
    this->mode = other.mode;
    this->meshNotFree = other.meshNotFree;
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
    this->mersList = other.mersList;
    this->geoType = other.geoType;*/
    this->unk1 = other.unk1;
    this->unk2 = other.unk2;
    this->unk3 = other.unk3;
    this->unk4 = other.unk4;
    this->unk5 = other.unk5;
    memcpy(this->fieldEnabled, other.fieldEnabled, sizeof(this->fieldEnabled));

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
    //this->geoType.clear();

    this->unk1 = 0xFF7FFFFF7F7FFFFF;
    this->unk2 = 0x7F7FFFFF7F7FFFFF;
    this->unk3 = 0x7F7FFFFF7F7FFFFF;
    this->unk4 = 0xFF7FFFFFFF7FFFFF;
    this->unk5 = 0xFF7FFFFFFF7FFFFF;

    memset(this->fieldEnabled, 0, sizeof(this->fieldEnabled));
}

void mce::MeshData::enableField(VertexField vertexField) {
    const int index = static_cast<int>(std::to_underlying(vertexField));

    if (index < 0 || index >= static_cast<int>(std::size(this->fieldEnabled)))
        return;

    this->fieldEnabled[index] = true;
}

bool mce::MeshData::hasField(VertexField vertexField) const {
    const int index = static_cast<int>(std::to_underlying(vertexField));

    if (index < 0 || index >= static_cast<int>(std::size(this->fieldEnabled)))
        return false;

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
