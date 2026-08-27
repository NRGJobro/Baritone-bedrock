#pragma once

struct ChunkLayerRenderObject {
    uint64_t chunkIdx;
    mce::MaterialPtr* material;
    uint32_t indicesStart;
    uint32_t indicesCount;
    uint32_t unsortedIndicesStart;
    uint32_t unsortedIndicesCount;
    bool shouldFallBackToUnsorted;
};
