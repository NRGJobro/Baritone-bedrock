#pragma once

#include "../../Block/TintMethod.h"
#include "../Biome.h"
#include "MapPolicy.h"

namespace BiomeColorSampling {
    enum class Pattern : int {
        Single,
        CornersAndMidpointsRange4,
        Grid3x3,
        CrossRange8
    };

    const std::vector<glm::ivec3>& getPattern(Pattern pattern);

    MapPolicy* getMapPolicy(TintMethod tintMethod);

    int getMapDefaultFoliageColor(Biome* biome, const glm::ivec3& pos);
    int getMapBirchFoliageColor(Biome* biome, const glm::ivec3& pos);
    int getMapEvergreenFoliageColor(Biome* biome, const glm::ivec3& pos);
    int getMapDryFoliageColor(Biome* biome, const glm::ivec3& pos);
    int getMapGrassColor(Biome* biome, const glm::ivec3& pos);
    int getMapWaterColor(Biome* biome, const glm::ivec3& pos);
};
