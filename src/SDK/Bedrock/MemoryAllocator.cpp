#include "MemoryAllocator.h"

void* Bedrock::Memory::MemoryAllocator::alignedAlloc(uint64_t size, uint64_t alignment) {
    alignment = std::min(alignment, static_cast<uint64_t>(8));

    if (size == 0)
        size = 1;

    auto ptr = malloc(alignment + 7 + size);
    auto ptr2 = ptr;

    if (ptr != nullptr) {
        ptr2 = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + alignment + 7 & ~(alignment - 1));
        *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(ptr2) - 8) = ptr;
    }

    return ptr2;
}

void Bedrock::Memory::MemoryAllocator::alignedRelease(void* ptr) {
    if (ptr != nullptr)
        free(ptr);
}
