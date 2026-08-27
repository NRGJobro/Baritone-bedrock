#include "BiomeColorSampling.h"
#include "BiomeAndPatternMapPolicy.h"

#include "WhiteMapPolicy.h"

const std::vector<glm::ivec3>& BiomeColorSampling::getPattern(const Pattern pattern) {
    switch (magic_enum::enum_integer(pattern)) {
        default:
        case 0: {
            static std::vector<glm::ivec3> singlePattern{};

            if (singlePattern.empty()) {
                singlePattern.resize(1);
                singlePattern[0] = {0, 0, 0};
            }

            return singlePattern;
        }
        case 1: {
            static std::vector<glm::ivec3> camr4Pattern{};

            if (camr4Pattern.empty()) {
                camr4Pattern.resize(8);

                int index = 0;
                for (int x = -1; x <= 1; x++) {
                    for (int z = -1; z <= 1; z++) {
                        if (x == 0 && z == 0)
                            continue;

                        camr4Pattern[index] = {x * 4, 0, z * 4};
                        index++;
                    }
                }
            }

            return camr4Pattern;
        }
        case 2: {
            static std::vector<glm::ivec3> gridPattern{};

            if (gridPattern.empty()) {
                gridPattern.resize(9);

                int index = 0;
                for (int x = -1; x <= 1; x++) {
                    for (int z = -1; z <= 1; z++) {
                        gridPattern[index] = {x, 0, z};
                        index++;
                    }
                }
            }

            return gridPattern;
        }
        case 3: {
            static std::vector<glm::ivec3> crossPattern{};

            if (crossPattern.empty()) {
                crossPattern.resize(5);

                crossPattern[0] = {-8, 0, -8};
                crossPattern[1] = {-8, 0, 8};
                crossPattern[2] = {8, 0, 8};
                crossPattern[3] = {8, 0, -8};
                crossPattern[4] = {0, 0, 0};
            }

            return crossPattern;
        }
    }
}

MapPolicy* BiomeColorSampling::getMapPolicy(const TintMethod tintMethod) {
    static std::vector<std::unique_ptr<MapPolicy>> policies{};

    if (policies.empty()) {
        policies.resize(magic_enum::enum_integer(TintMethod::Size));

        const auto camr4Pattern = getPattern(Pattern::CornersAndMidpointsRange4);
        const auto gridPattern = getPattern(Pattern::Grid3x3);
        const auto crossPattern = getPattern(Pattern::CrossRange8);

        policies[0] = std::make_unique<WhiteMapPolicy>();
        policies[1] = std::make_unique<BiomeAndPatternMapPolicy>(getMapDefaultFoliageColor, camr4Pattern);
        policies[2] = std::make_unique<BiomeAndPatternMapPolicy>(getMapBirchFoliageColor, camr4Pattern);
        policies[3] = std::make_unique<BiomeAndPatternMapPolicy>(getMapEvergreenFoliageColor, camr4Pattern);
        policies[4] = std::make_unique<BiomeAndPatternMapPolicy>(getMapDryFoliageColor, camr4Pattern);
        policies[5] = std::make_unique<BiomeAndPatternMapPolicy>(getMapGrassColor, crossPattern);
        policies[6] = std::make_unique<BiomeAndPatternMapPolicy>(getMapWaterColor, gridPattern);
        policies[7] = std::make_unique<WhiteMapPolicy>();
        policies[8] = std::make_unique<WhiteMapPolicy>();
    }

    const int i = magic_enum::enum_integer(tintMethod);

    if (i < 0 || i >= magic_enum::enum_integer(TintMethod::Size))
        return nullptr;

    return policies[i].get();
}

int BiomeColorSampling::getMapDefaultFoliageColor(Biome* biome, const glm::ivec3& pos) {
    return biome->getMapFoliageColor();
}

int BiomeColorSampling::getMapBirchFoliageColor(Biome* biome, const glm::ivec3& pos) {
    return 0x80A755;
}

int BiomeColorSampling::getMapEvergreenFoliageColor(Biome* biome, const glm::ivec3& pos) {
    return 0x619961;
}

int BiomeColorSampling::getMapDryFoliageColor(Biome* biome, const glm::ivec3& pos) {
    return 0x5C3C32;
}

int BiomeColorSampling::getMapGrassColor(Biome* biome, const glm::ivec3& pos) {
    return biome->getMapGrassColor(pos);
}

int BiomeColorSampling::getMapWaterColor(Biome* biome, const glm::ivec3& pos) {
    return biome->getMapWaterColor();
}
