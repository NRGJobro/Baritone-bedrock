#include "MapUtils.h"

#include "DrawUtils.h"

void MapUtils::tessellateMapSampleSimple(const ChunkSample& sample, const ColorMode mode, const float startX, const float startY, const float cellSize, const int16_t minHeight, const int16_t maxHeight) {
    // Get the appropriate color data
    std::vector<mce::Color> colors;

    if (mode == ColorMode::Heatmap)
        colors = sample.getHeightDataAsHeatmap(minHeight, maxHeight);
    else if (mode == ColorMode::Greyscale)
        colors = sample.getHeightDataAsGreyscale(minHeight, maxHeight);
    else if (mode == ColorMode::LightLevels)
        colors = sample.getLightLevelsAsGreyscale();
    else
        colors = sample.getColors();

    // Calculate grid dimensions (assuming square grid)
    int gridSize = static_cast<int>(std::sqrt(colors.size()));

    if (gridSize * gridSize != colors.size())
        gridSize = static_cast<int>(colors.size()); // fallback for non-square data

    // Render each cell as a filled rectangle
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            const int index = x * gridSize + y;

            if (index >= colors.size())
                continue;

            const mce::Color& color = colors[index];

            // Calculate rectangle position
            const float rectX = startX + static_cast<float>(x) * cellSize;
            const float rectY = startY + static_cast<float>(y) * cellSize;
            glm::vec4 pos(rectX, rectY, rectX + cellSize, rectY + cellSize);

            // Render the rectangle
            DrawUtils::addFilledRectangle(pos, color, 1.f);
        }
    }
}
