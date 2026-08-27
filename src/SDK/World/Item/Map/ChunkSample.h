#pragma once

#include "../../../Client/MCE/Color.h"
#include "../../Level/BrightnessPair.h"
#include "../../Level/Chunk/ChunkLocalHeight.h"
#include "../../Level/Chunk/ChunkPos.h"

// Custom class
struct ChunkSample {
    ChunkPos pos;
    std::vector<mce::Color> colors; // will be divided by blocksPerTexel
    std::vector<ChunkLocalHeight> heights;
    std::vector<BrightnessPair> lightLevels;

    ChunkSample(ChunkPos pos, int blocksPerTexel);

    [[nodiscard]] const std::vector<mce::Color>& getColors() const;
    [[nodiscard]] std::vector<mce::Color> getLightLevelsAsGreyscale() const;
    [[nodiscard]] std::vector<mce::Color> getHeightDataAsGreyscale(int16_t minHeight, int16_t maxHeight) const;
    [[nodiscard]] std::vector<mce::Color> getHeightDataAsHeatmap(int16_t minHeight, int16_t maxHeight) const;
};
