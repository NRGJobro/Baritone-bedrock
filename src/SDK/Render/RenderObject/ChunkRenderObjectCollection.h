#pragma once

#include "ChunkLayerRenderObject.h"
#include "ChunkRenderData.h"
#include "../../Client/mce/ServerTexture.h"

struct ChunkRenderObjectCollection {
    std::vector<mce::ServerTexture> textures;
    std::vector<ChunkRenderData> chunkQueue;
    std::vector<ChunkLayerRenderObject> terrainLayerChunkQueue[3][19];
    uint32_t maximumChunkVertexCount;
};
