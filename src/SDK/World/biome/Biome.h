#pragma once

#include "../BlockSource.h"
#include "../../Client/MCE/Color.h"

class Biome;

using SamplerFunc = int(*)(Biome*, const glm::ivec3&);

class Biome {
public:
    int getMapWaterColor();
    int getMapFoliageColor();
    int getMapGrassColor(const glm::ivec3& pos);

    static mce::Color getColorBySamplingSurroundings(BlockSource* region, const glm::ivec3& pos, const std::vector<glm::ivec3>& pattern, SamplerFunc sampler);
};
