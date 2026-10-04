#include "MapItem.h"

#include "../../../Memory/Sig/SignatureManager.h"
#include "../../../Utils/Utils.h"
#include "../../MC.h"
#include "../Level/Chunk/ChunkSource.h"
#include "../Level/Chunk/LevelChunk.h"
#include "Map/MapBrightness.h"

bool MapItem::sampleMapData(BlockSource* region, int blocksPerTexel, const glm::ivec3& worldOrigin, const glm::ivec3& updateOrigin, int imageWidth, int imageHeight, std::vector<MapSample>* output, MapItemSavedData* mapData, std::vector<ClientTerrainPixel>* pixels) {
    static auto sig = Utils::getFromOffset<uintptr_t>(GET_SIG("MapItem::sampleMapData"), 1);
    static auto func = *(decltype(&MapItem::sampleMapData)*)&sig;
    return func(region, blocksPerTexel, worldOrigin, updateOrigin, imageWidth, imageHeight, output, mapData, pixels);
}

void MapItem::updateChunkSamples(std::vector<ChunkSample>& samples, int blocksPerTexel) {
    const auto clientInstance = MC::getClientInstance();
    const auto player = clientInstance->getLocalPlayer();

    if (player == nullptr)
        return;

    const auto dimension = player->getDimension();
    const auto blockSource = clientInstance->getBlockSource();

    // Get player position and convert to chunk coordinates
    const auto& playerPos = player->tryGet<StateVectorComponent>()->pos;
    const glm::ivec2 playerChunkPos(playerPos.x / 16, playerPos.z / 16);

    const int minHeight = dimension->getMinHeight();

    // Calculate chunk radius based on blocksPerTexel
    int chunkRadius = 3;
    int sampleRadius = chunkRadius;

    if (blocksPerTexel > 1)
        chunkRadius = 4 / blocksPerTexel;

    chunkRadius = std::max(1, chunkRadius);

    samples.clear();

    // Track previous average height for brightness calculation
    for (int dx = -sampleRadius; dx <= chunkRadius; dx++) {
        for (int dz = -sampleRadius; dz <= chunkRadius; dz++) {
            ChunkPos chunkPos(playerChunkPos.x + dx, playerChunkPos.y + dz);
            const auto chunk = dimension->getChunkSource()->getAvailableChunk(chunkPos);

            if (chunk == nullptr)
                continue;

            ChunkSample sample(chunkPos, blocksPerTexel);
            const auto& heightmap = chunk->getHeightmap();

            // Initialize previous heights for this chunk
            int gridSize = 16 / blocksPerTexel;
            std::vector chunkPreviousHeights(gridSize * gridSize, 0.f);

            for (int x = 0; x < 16; x += blocksPerTexel) {
                for (int z = 0; z < 16; z += blocksPerTexel) {
                    // Track average height and water depth for this blocksPerTexel × blocksPerTexel area
                    float averageHeight = 0.f;
                    int waterDepthTotal = 0;
                    mce::Color dominantColor(0, 0, 0, 0);
                    bool isWaterDominant = false;
                    BrightnessPair lightData{};
                    BrightnessPair lightDataWater{};

                    int gridX = x / blocksPerTexel;
                    int gridZ = z / blocksPerTexel;
                    int gridIndex = gridZ * gridSize + gridX;

                    // Get previous average height (from above)
                    float previousAverageHeight = 0.f;
                    // Sample the bottom blocks of the north chunk
                    float northHeightSum = 0.f;
                    int northSampleCount = 0;

                    if (gridZ > 0)
                        previousAverageHeight = chunkPreviousHeights[(gridZ - 1) * gridSize + gridX];

                    // If no above neighbor, try to get from chunk to the north
                    else if (dz > -(sampleRadius + 1)) {
                        ChunkPos northChunkPos(playerChunkPos.x + dx, playerChunkPos.y + dz - 1);
                        const auto northChunk = dimension->getChunkSource()->getAvailableChunk(northChunkPos);

                        if (northChunk != nullptr) {
                            // Get the bottom row from the north chunk
                            const auto& northHeightmap = northChunk->getHeightmap();

                            for (int blockX = 0; blockX < blocksPerTexel; blockX++) {
                                int sampleX = x + blockX;
                                int sampleZ = 15; // Bottom z in north chunk

                                if (sampleX >= 16)
                                    continue;

                                int surfaceHeight = northHeightmap[sampleZ * 16 + sampleX].val + minHeight - 1;
                                northHeightSum += surfaceHeight;
                                northSampleCount++;
                            }
                        }
                    }

                    mce::Color underwaterBlockColor{};

                    for (int blockX = 0; blockX < blocksPerTexel; blockX++) {
                        for (int blockZ = 0; blockZ < blocksPerTexel; blockZ++) {
                            int sampleX = x + blockX;
                            int sampleZ = z + blockZ;

                            if (sampleX >= 16 || sampleZ >= 16)
                                continue;

                            // Get the surface height from heightmap
                            int surfaceHeight = heightmap[sampleZ * 16 + sampleX].val + minHeight - 1;
                            ChunkBlockPos cbp(sampleX, sampleZ, surfaceHeight);
                            const auto rainPos = chunk->getTopRainBlockPos(cbp);

                            surfaceHeight = std::max(surfaceHeight + 1, rainPos.y);

                            mce::Color sampleColor{};
                            Block* block = nullptr;

                            glm::ivec3 worldPos(chunkPos->x * 16 + sampleX, surfaceHeight, chunkPos->y * 16 + sampleZ);

                            // Start from the highest point and go downward to find first non-black block
                            bool foundNonBlack = false;
                            for (int y = surfaceHeight; y >= minHeight; y--) {
                                glm::ivec3 checkPos(chunkPos->x * 16 + sampleX, y, chunkPos->y * 16 + sampleZ);
                                const auto currentBlock = blockSource->getBlock(checkPos);

                                if (currentBlock != nullptr && currentBlock->getBlockLegacy()) {
                                    // Get the block's color
                                    const auto blockColor = currentBlock->getMapColor(blockSource, checkPos);

                                    // Check if the block is not black (you might want to adjust the threshold)
                                    if (blockColor.r > 0.f || blockColor.g > 0.f || blockColor.b > 0.f) {
                                        // Found non-black block
                                        block = currentBlock;
                                        surfaceHeight = y;
                                        sampleColor = blockColor;
                                        foundNonBlack = true;
                                        break;
                                    }
                                }
                            }

                            // If no non-black block found, use the original surface block
                            if (!foundNonBlack) {
                                block = blockSource->getBlock(worldPos);
                                sampleColor = block->getMapColor(blockSource, worldPos);
                            }

                            averageHeight += surfaceHeight;

                            if (block != nullptr && block->getBlockLegacy()) {
                                const auto blockLegacy = block->getBlockLegacy();

                                // Calculate subchunk index and local position
                                const int subChunkIndex = (surfaceHeight - minHeight + 1) / 16;

                                // Ensure we don't go out of bounds
                                if (subChunkIndex < 0 || subChunkIndex >= chunk->getSubChunks().size())
                                    continue;

                                const auto& subChunk = chunk->getSubChunks()[subChunkIndex];
                                const int localYPos = (surfaceHeight - minHeight + 1) % 16;

                                // Create local position within the subchunk
                                glm::ivec3 localPos(sampleX, localYPos, sampleZ);

                                // Get brightness data using local position
                                lightData = subChunk.getLightLevelAt(localPos);

                                // Simple dominance tracking
                                // TODO: Currently if plant is waterlogged and at the top of water surface, check will fail (will just count as plant)
                                if (blockLegacy->getMaterial()->isType(MaterialType::Water)) { // Water block ID
                                    isWaterDominant = true;

                                    // Calculate water depth
                                    int waterDepth = 0;
                                    int fluidDepthCheck = surfaceHeight - 1;

                                    lightDataWater = lightData;

                                    while (fluidDepthCheck > 0) {
                                        const auto checkPos = glm::ivec3{worldPos.x, fluidDepthCheck, worldPos.z};
                                        const auto belowBlock = blockSource->getBlock(checkPos);

                                        const auto material = belowBlock->getBlockLegacy()->getMaterial();

                                        if (belowBlock == nullptr || !material->isType(MaterialType::Water) &&
                                            !material->isType(MaterialType::Plant) &&
                                            !material->isType(MaterialType::ReplaceablePlant)) {
                                            underwaterBlockColor = belowBlock->getMapColor(blockSource, checkPos);

                                            // Calculate subchunk index and local position
                                            const int subChunkIndex_Depth = (fluidDepthCheck - minHeight + 1) / 16;

                                            // Ensure we don't go out of bounds
                                            if (subChunkIndex_Depth < 0 || subChunkIndex_Depth >= chunk->getSubChunks().size())
                                                break;

                                            const auto& subChunk_Depth = chunk->getSubChunks()[subChunkIndex];
                                            const int localYPos_Depth = fluidDepthCheck % 16;

                                            // Create local position within the subchunk
                                            glm::ivec3 localPos_Depth(sampleX, localYPos_Depth, sampleZ);

                                            // Get brightness data using local position
                                            lightData = subChunk_Depth.getLightLevelAt(localPos_Depth);

                                            // Can be used to control minimum brightness on underwater blocks
                                            lightData.sky.value = 128;
                                            break;
                                        }

                                        waterDepth++;
                                        fluidDepthCheck--;
                                    }

                                    waterDepthTotal += waterDepth;
                                }

                                if (northSampleCount > 0)
                                    previousAverageHeight = (northHeightSum - waterDepthTotal) / northSampleCount;

                                averageHeight -= waterDepthTotal;

                                dominantColor = sampleColor; // Simple approach
                            }
                        }
                    }

                    // Calculate averages
                    int totalSamples = blocksPerTexel * blocksPerTexel;
                    averageHeight /= totalSamples;
                    int averageWaterDepth = waterDepthTotal / totalSamples;

                    // Store current height for next iteration
                    chunkPreviousHeights[gridIndex] = averageHeight;

                    // Apply brightness based on block type
                    MapBrightness::Value brightness = MapBrightness::Normal;
                    MapBrightness::Value waterBrightness = MapBrightness::Normal;

                    mce::Color waterColor{};

                    if (isWaterDominant) {
                        // Water brightness based on depth and pattern
                        const float depthFactor = averageWaterDepth * 0.1f + ((x + z) & 1) * 0.2f;

                        if (depthFactor < 0.5f)
                            waterBrightness = MapBrightness::High;
                        else if (depthFactor > 0.9f)
                            waterBrightness = MapBrightness::Low;

                        const float waterModifier = static_cast<float>(MapBrightness::getModifier(waterBrightness)) / 255.f;

                        waterColor = mce::Color(
                                dominantColor.r * waterModifier,
                                dominantColor.g * waterModifier,
                                dominantColor.b * waterModifier,
                                dominantColor.a
                        );

                        dominantColor = underwaterBlockColor;
                    }

                    // Terrain brightness based on height variation
                    const float brightnessFactor = (averageHeight - previousAverageHeight) * 4.f / (blocksPerTexel + 4) + (((x + z) & 1) - 0.5f) * 0.4f;

                    if (brightnessFactor > 0.6f)
                        brightness = MapBrightness::High;
                    else if (brightnessFactor < -0.6f)
                        brightness = MapBrightness::Low;

                    // Apply brightness to color
                    const float modifier = static_cast<float>(MapBrightness::getModifier(brightness)) / 255.f;
                    mce::Color finalColor(
                            dominantColor.r * modifier,
                            dominantColor.g * modifier,
                            dominantColor.b * modifier,
                            dominantColor.a
                    );

                    const auto lightTexture = clientInstance->getLightTexture();
                    const auto lightColor = lightTexture->getColorForUV(lightData);
                    const auto waterLightColor = lightTexture->getColorForUV(lightDataWater);

                    if (isWaterDominant)
                        finalColor = (finalColor * lightColor).mix(waterColor * waterLightColor, 0.5f);
                    else
                        finalColor = finalColor * lightColor;

                    sample.colors.push_back(finalColor);
                    sample.heights.push_back(ChunkLocalHeight{static_cast<int16_t>(averageHeight)});
                    sample.lightLevels.push_back(lightData);
                }
            }

            samples.push_back(std::move(sample));
        }
    }
}
