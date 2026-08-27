#pragma once

#include "../../Client/mce/Mesh.h"
#include "../../Client/mce/PointLight.h"

struct ChunkRenderData {
    glm::vec3 position;
    double readyTimeDiff;
    std::variant<std::monostate, std::shared_ptr<mce::IndexBufferContainer>, std::shared_ptr<int64_t>> chunkIndices;
    std::variant<std::monostate, std::shared_ptr<mce::Mesh>, std::shared_ptr<int64_t>> chunkMesh;
    std::span<mce::PointLight> pointLights;
};
