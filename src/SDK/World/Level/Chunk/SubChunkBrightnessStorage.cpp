#include "SubChunkBrightnessStorage.h"

Brightness SubChunkBrightnessStorage::getLightLevelAt(const glm::ivec3 localPos) const {
    const size_t index = localPos.x * 16 * 16 + localPos.z * 16 + localPos.y;
    const size_t lightPairIndex = index / 2;

    return lightValues[lightPairIndex];
}
