#pragma once

#include "MemBlock.h"

struct AllocatorData {
    uint64_t totalBlocks;
    uint64_t totalCapacity;
    uint64_t totalSize;
    std::forward_list<MemBlock> blocks;
};
