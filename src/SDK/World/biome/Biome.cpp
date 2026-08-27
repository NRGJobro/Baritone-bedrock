#include "Biome.h"

#include "../../../Memory/Sig/SignatureManager.h"

int Biome::getMapWaterColor() {
    return hat::member_at<int>(this, 0x38);
}

int Biome::getMapFoliageColor() {
    static auto sig = GET_SIG("Biome::getMapFoliageColor");
    using func_t = int(*)(Biome*);
    static auto func = reinterpret_cast<func_t>(sig);
    return func(this);
}

int Biome::getMapGrassColor(const glm::ivec3& pos) {
    static auto sig = GET_SIG("Biome::getMapGrassColor");
    using func_t = int(*)(Biome*, const glm::ivec3&);
    static auto func = reinterpret_cast<func_t>(sig);
    return func(this, pos);
}

mce::Color Biome::getColorBySamplingSurroundings(BlockSource* region, const glm::ivec3& pos, const std::vector<glm::ivec3>& pattern, const SamplerFunc sampler) {
    if (pattern.empty())
        return {0.f, 0.f, 0.f, 0.f};

    mce::Color out{0.f, 0.f, 0.f, 0.f};

    for (const auto& p : pattern) {
        const auto tmp = pos + p;
        const auto biome = region->getBiome(tmp);

        if (biome == nullptr)
            return {0.f, 0.f, 0.f, 0.f};

        const int col = sampler(biome, tmp);

        out.r += static_cast<float>(col >> 16 & 0xFF);
        out.g += static_cast<float>(col >> 8 & 0xFF);
        out.b += static_cast<float>(col & 0xFF);
        out.a += static_cast<float>(col >> 24 & 0xFF);
    }

    const float mul = 1.f / (static_cast<float>(pattern.size()) * 255.f);

    out.r *= mul;
    out.g *= mul;
    out.b *= mul;

    out.r = std::clamp(out.r, 0.f, 1.f);
    out.g = std::clamp(out.g, 0.f, 1.f);
    out.b = std::clamp(out.b, 0.f, 1.f);
    out.a = std::clamp(out.a, 0.f, 1.f);

    return out;
}
