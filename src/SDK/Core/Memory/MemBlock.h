#pragma once

struct MemBlock {
    uint64_t capacity;
    uint64_t size;
    std::unique_ptr<uint8_t[0]> dataBlock;
};
