#include "BiomeAndPatternMapPolicy.h"

BiomeAndPatternMapPolicy::BiomeAndPatternMapPolicy(const SamplerFunc func, const std::vector<glm::ivec3>& patterns) {
    this->func = func;
    this->patterns = patterns;
}

mce::Color BiomeAndPatternMapPolicy::get(BlockSource* region, const glm::ivec3& pos) {
    return Biome::getColorBySamplingSurroundings(region, pos, this->patterns, this->func);
}
