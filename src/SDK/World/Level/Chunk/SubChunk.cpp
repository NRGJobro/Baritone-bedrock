#include "SubChunk.h"

BrightnessPair SubChunk::getLightLevelAt(const glm::ivec3 localPos) const {
    BrightnessPair data{};

    // Check if position is within subchunk bounds (typically 0-15 in all dimensions)
    if (localPos.x < 0 || localPos.x >= 16 ||
        localPos.y < 0 || localPos.y >= 16 ||
        localPos.z < 0 || localPos.z >= 16) {
        return data;
    }

    if (hasMaxSkyLight) {
        data.sky.value = 255;
        data.block.value = 0;
    }

    if (!skyLight && hasMaxSkyLight)
        data.sky.value = 255;
    else if (skyLight)
        data.sky = skyLight->getLightLevelAt(localPos);

    if (blockLight)
        data.block = blockLight->getLightLevelAt(localPos);

    return data;
}
