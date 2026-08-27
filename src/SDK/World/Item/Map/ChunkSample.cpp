#include "ChunkSample.h"

ChunkSample::ChunkSample(const ChunkPos pos, const int blocksPerTexel) {
    this->pos = pos;
    this->colors.reserve(256 / blocksPerTexel);
    this->heights.reserve(256 / blocksPerTexel);
    this->lightLevels.reserve(256 / blocksPerTexel);
}

const std::vector<mce::Color>& ChunkSample::getColors() const {
    return this->colors;
}

std::vector<mce::Color> ChunkSample::getLightLevelsAsGreyscale() const {
    std::vector<mce::Color> result;
    result.reserve(this->lightLevels.size());

    // Convert each height value to grayscale
    for (const auto& h : this->lightLevels) {
        float normalizedBlock = static_cast<float>(h.block.value) / static_cast<float>(Brightness::MAX().value);
        float normalizedSky = static_cast<float>(h.sky.value) / static_cast<float>(Brightness::MAX().value);
        result.emplace_back(normalizedSky, normalizedBlock, normalizedBlock, 1.f);
    }

    return result;
}

std::vector<mce::Color> ChunkSample::getHeightDataAsGreyscale(const int16_t minHeight, const int16_t maxHeight) const {
    std::vector<mce::Color> result;
    result.reserve(this->heights.size());

    // Avoid division by zero
    if (minHeight == maxHeight) {
        for (int i = 0; i < this->heights.size(); i++) {
            result.emplace_back(0.5f, 0.5f, 0.5f, 1.f);
        }

        return result;
    }

    // Convert each height value to grayscale
    for (const auto& h : this->heights) {
        float normalized = static_cast<float>(h.val - minHeight) / static_cast<float>(maxHeight - minHeight);
        result.emplace_back(normalized, normalized, normalized, 1.f);
    }

    return result;
}

std::vector<mce::Color> ChunkSample::getHeightDataAsHeatmap(const int16_t minHeight, const int16_t maxHeight) const {
    std::vector<mce::Color> result;
    result.reserve(this->heights.size());

    // Avoid division by zero
    if (minHeight == maxHeight) {
        for (int i = 0; i < this->heights.size(); i++) {
            result.emplace_back(1.f, 0.f, 0.f, 1.f); // Red for uniform height
        }

        return result;
    }

    // Convert each height value to heatmap color
    for (const auto& h : this->heights) {
        const float normalized = static_cast<float>(h.val - minHeight) / static_cast<float>(maxHeight - minHeight);

        // Heatmap color mapping: blue (low) -> cyan -> green -> yellow -> red (high)
        float r, g, b;

        if (normalized < 0.25f) {
            // Blue to cyan
            r = 0.f;
            g = normalized * 4.f;
            b = 1.f;
        }
        else if (normalized < 0.5f) {
            // Cyan to green
            r = 0.f;
            g = 1.f;
            b = 1.f - (normalized - 0.25f) * 4.0f;
        }
        else if (normalized < 0.75f) {
            // Green to yellow
            r = (normalized - 0.5f) * 4.f;
            g = 1.f;
            b = 0.f;
        }
        else {
            // Yellow to red
            r = 1.f;
            g = 1.f - (normalized - 0.75f) * 4.f;
            b = 0.f;
        }

        result.emplace_back(r, g, b, 1.f);
    }

    return result;
}
