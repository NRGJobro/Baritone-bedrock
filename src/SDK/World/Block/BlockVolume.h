#pragma once

#include "../../Core/Memory/buffer_span_mut.h"
#include "Block.h"

class BlockVolume {
public:
    buffer_span_mut<Block*> blocks;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    int dimensionBottom;
    Block* initBlock = nullptr;

    BlockVolume(const glm::ivec3& volume, int16_t dimensionBottom);
};
