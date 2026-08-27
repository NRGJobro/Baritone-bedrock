#pragma once

#include "MapPolicy.h"
#include "../Biome.h"

class BiomeAndPatternMapPolicy : public MapPolicy {
    SamplerFunc func;
    std::vector<glm::ivec3> patterns;

public:
    BiomeAndPatternMapPolicy(SamplerFunc func, const std::vector<glm::ivec3>& patterns);

    mce::Color get(BlockSource* region, const glm::ivec3& pos) override;
};
