#include "Tessellator.h"

#include "../../Utils/TimeUtils.h"
#include "../../Utils/Utils.h"

void Tessellator::begin(const mce::PrimitiveMode mode, const int maxVertices, const bool buildFaceData) {
    //if (!this->tessellating && !this->overriden) {
        this->clear();

        this->meshData.mode = mode;
        this->buildFaceData = buildFaceData;
        this->noColor = false;
        this->tessellating = true;
        this->currQuadIndex = 0;

        this->quadInfoList.clear();

        this->meshData.enableField(mce::VertexField::Position);

        if (maxVertices > 0) {
            this->meshData.reserveVertices(maxVertices);

            //if (this->buildFaceData && mode != mce::PrimitiveMode::None)
            //    this->quadInfoList.reserve(maxVertices / getFaceVertexCountForMode(mode) + 1);
        }
    //}
}

void Tessellator::vertex(float x, float y, float z) {
    if (this->count != this->maxFaces) {
        this->count++;
        this->isFormatFixed = true;

        /*if (this->applyTransform) {
            glm::vec4 vec{x, y, z, 0.f};
            vec = this->transformMatrix * vec;

            x = vec.x;
            y = vec.y;
            z = vec.z;
        }

        {
            const auto& transformationOffset = this->postTransformationOffset;
            const auto& transformationScale = this->postTransformationScale;

            x = x * transformationScale.x + transformationOffset.x;
            y = y * transformationScale.y + transformationOffset.y;
            z = z * transformationScale.z + transformationOffset.z;
        }*/

        auto& meshData = this->meshData;

        meshData.positions.emplace_back(x, y, z);

        //if (this->nextNormal.has_value())
        //    meshData.normals.emplace_back(this->nextNormal.value());

        if (this->nextColor.has_value())
            meshData.colors.emplace_back(this->nextColor.value());

        if (this->nextUV[0].has_value())
            meshData.textureUVs[0].emplace_back(this->nextUV[0].value());

        /*if (this->nextBoneId.has_value())
            meshData.boneId0s.emplace_back(this->nextBoneId.value());

        for (int i = 0; i < 3; i++) {
            if (this->nextUV[i].has_value())
                meshData.textureUVs[i].emplace_back(this->nextUV[i].value());
        }

        if (this->nextPBRTextureIdx.has_value())
            meshData.pbrTextureIndices.emplace_back(this->nextPBRTextureIdx.value());

        if (this->nextMers.has_value())
            meshData.mersList.emplace_back(this->nextMers.value());

        if (this->buildFaceData) {
            int vertexCount;

            switch (meshData.mode) {
                case mce::PrimitiveMode::QuadList: {
                    vertexCount = 4;
                    break;
                }
                case mce::PrimitiveMode::TriangleStrip: {
                    vertexCount = 3;
                    break;
                }
                case mce::PrimitiveMode::LineStrip: {
                    vertexCount = 2;
                    break;
                }
                default: {
                    vertexCount = 0;
                    break;
                }
            }

            {
                auto& faceCenterAccumulator = this->faceCenterAccumulator;

                x += faceCenterAccumulator.x;
                y += faceCenterAccumulator.y;
                z += faceCenterAccumulator.z;

                faceCenterAccumulator.x = x;
                faceCenterAccumulator.y = y;
                faceCenterAccumulator.z = z;
            }

            this->currQuadIndex++;

            if (this->currQuadIndex == vertexCount) {
                this->currQuadIndex = 0;

                const float mul = 1.f / static_cast<float>(vertexCount);

                TessellatorQuadInfo info;

                info.facing = this->quadFacing;
                info.twoFace = this->quadTwoSided;
                info.centroid = {x * mul, y * mul, z * mul};

                this->quadInfoList.emplace_back(info);

                this->faceCenterAccumulator = {};
                this->quadFacing = 6;
                this->quadTwoSided = false;
            }
        }*/
    }
}

void Tessellator::vertex(const glm::vec3& pos) {
    this->vertex(pos.x, pos.y, pos.z);
}

void Tessellator::vertexChroma(const float x, const float y, const float alpha) {
    const auto l = TimeUtils::currentTimeMillis() - (static_cast<millis>(x) * 10 - static_cast<millis>(y) * 10);
    const auto c = Utils::HSVtoRGB(static_cast<float>(l % 2000) / 2000.f, 0.8f, 0.8f);

    this->color(c, alpha);
    this->vertex(x, y, 0.f);
}

void Tessellator::vertexUV(const float x, const float y, const float z, float u, float v) {
    //u = std::clamp(u, 0.f, 1.f);
    //v = std::clamp(v, 0.f, 1.f);

    this->nextUV[0] = {u, v};

    if (!this->isFormatFixed)
        this->meshData.enableField(mce::VertexField::UV0);

    this->vertex(x, y, z);
}

void Tessellator::color(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a) {
    //if (!this->noColor) {
        this->nextColor = a << 24 | b << 16 | g << 8 | r;

        if (!this->isFormatFixed)
            this->meshData.enableField(mce::VertexField::Color);
    //}
}

void Tessellator::color(const float r, const float g, const float b, const float a) {
    this->color(static_cast<uint8_t>(r * 255.f), static_cast<uint8_t>(g * 255.f), static_cast<uint8_t>(b * 255.f), static_cast<uint8_t>(a * 255.f));
}

void Tessellator::color(const mce::Color &color) {
    this->color(color.r, color.g, color.b, color.a);
}

void Tessellator::color(const mce::Color &color, const float alpha) {
    this->color(color.r, color.g, color.b, alpha);
}

void Tessellator::end(mce::Mesh& mesh) {
    //if (this->tessellating && !this->overriden) {
        /*if (this->meshData.positions.empty()) {
            this->tessellating = false;
            return;
        }*/

        if (this->bufferResourceService.lock() != nullptr) {
            mesh.temporary = true;
            mesh.primitiveMode = this->meshData.mode;
            mesh.bufferResourceService = this->bufferResourceService.lock();
            mesh.meshData = this->meshData;
        }

        this->clear();
    //}
}

void Tessellator::clear() {
    this->count = 0;
    this->overriden = false;
    this->tessellating = false;
    this->indexPhase = false;
    this->meshData.clear();
    this->isFormatFixed = false;
    this->hasNormals = false;

    //this->nextNormal.reset();
    this->nextColor.reset();
    //this->nextBoneId.reset();
    //this->nextPBRTextureIdx.reset();
    //this->nextMers.reset();

    //for (auto& opt : this->nextUV) {
    //    opt.reset();
    //}

    this->nextUV[0].reset();
}

size_t Tessellator::getFaces() const {
    return this->meshData.positions.size() / getFaceVertexCountForMode(this->meshData.mode);
}

size_t Tessellator::getVertices() const {
    return this->meshData.positions.size();
}

bool Tessellator::isTessellating() const {
    return this->tessellating;
}

int Tessellator::getFaceVertexCountForMode(const mce::PrimitiveMode mode) {
    switch (mode) {
        case mce::PrimitiveMode::QuadList:
            return 4;
        case mce::PrimitiveMode::TriangleList:
        case mce::PrimitiveMode::TriangleStrip:
            return 3;
        case mce::PrimitiveMode::LineList:
        case mce::PrimitiveMode::LineStrip:
            return 2;
        default:
            return 0;
    }
}
