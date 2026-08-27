#pragma once

#include "../Core/AutomaticID.h"
#include "BlockSource.h"
#include "DimensionID.h"
#include "Level/Level.h"

class ChunkSource;

class Dimension {
public:
    Level* getLevel();
    int16_t getMinHeight();
    int16_t getMaxHeight();
    std::shared_ptr<BlockSource> getBlockSource();
    DimensionID getDimensionID();
    bool hasCeiling();
    ChunkSource* getChunkSource();
    float getTimeOfDay(int ticks, float a);

    float getTimeOfDay(float a = 1.f);
};

using DimensionType = AutomaticID<Dimension, int16_t>;
