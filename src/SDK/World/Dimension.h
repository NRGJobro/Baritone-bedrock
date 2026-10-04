#pragma once

#include "BlockSource.h"

class Dimension {
public:
    int16_t getMinHeight();
    std::shared_ptr<BlockSource> getBlockSource();
};
