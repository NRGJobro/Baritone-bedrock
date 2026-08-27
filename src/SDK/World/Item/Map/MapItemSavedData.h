#pragma once

#include "../../Dimension.h"
#include "ClientTerrainPixel.h"
#include "MapItemTrackedActor.h"

struct MapItemSavedData {
    uint64_t updateInterval;
    int64_t mapId;
    int64_t parentMapId;
    bool isFullyExplored;
    bool previewIncomplete;
    glm::ivec3 origin;
    DimensionType dimension;
    int8_t scale;
    std::vector<uint32_t> pixels;
    std::vector<ClientTerrainPixel> clientPixels;
    std::vector<std::shared_ptr<MapItemTrackedActor>> trackedEntities;
    bool unlimitedTracking;
    bool dirtyForSave;
    bool dirtyPixelData;
    bool locked;
    std::vector<std::pair<MapItemTrackedActor::UniqueId, std::shared_ptr<MapDecoration>>> decorations;
    bool hasDirtyClientPixels;
    std::unique_ptr<void*> clientSamplingLock;
    bool needsResampling;
    bool isDLCWorld;
};
