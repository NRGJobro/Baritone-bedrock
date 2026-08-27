#pragma once

#include "Map/ChunkSample.h"
#include "Map/ClientTerrainPixel.h"
#include "Map/MapItemSavedData.h"
#include "Map/MapSample.h"

class MapItem {
public:
    static bool sampleMapData(BlockSource* region, int blocksPerTexel, const glm::ivec3& worldOrigin, const glm::ivec3& updateOrigin, int imageWidth, int imageHeight, std::vector<MapSample>* output, MapItemSavedData* mapData, std::vector<ClientTerrainPixel>* pixels);

    // Custom function
    static void updateChunkSamples(std::vector<ChunkSample>& samples, int blocksPerTexel);
};
