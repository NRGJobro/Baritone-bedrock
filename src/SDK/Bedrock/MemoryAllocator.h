#pragma once

namespace Bedrock::Memory {
    class MemoryAllocator {
    public:
        static void* alignedAlloc(uint64_t size, uint64_t alignment);
        static void alignedRelease(void* ptr);
    };
}
