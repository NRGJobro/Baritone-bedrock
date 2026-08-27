#pragma once

#include "../SDK/World/Item/Map/ChunkSample.h"

class MapUtils {
public:
    enum class ColorMode : uint8_t {
        Normal,
        Heatmap,
        Greyscale,
        LightLevels
    };

    static void tessellateMapSampleSimple(const ChunkSample& sample, ColorMode mode = ColorMode::Normal, float startX = 0.f, float startY = 0.f, float cellSize = 10.f, int16_t minHeight = 0, int16_t maxHeight = 0);
};
