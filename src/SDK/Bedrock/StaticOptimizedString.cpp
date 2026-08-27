#include "StaticOptimizedString.h"

#include "MemoryAllocator.h"

void Bedrock::StaticOptimizedString::_set(const char* data, const uint64_t length, const StorageType storageType) {
    if (data != nullptr && (storageType == StorageType::Dynamic || length < 0x7F)) {
        const auto ptr = static_cast<uint64_t*>(Memory::MemoryAllocator::alignedAlloc(length + 9, 8));
        const auto destination = ptr + 1;

        *ptr = length;

        memcpy(destination, data, length);

        *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(destination) + length) = 0;

        this->data = reinterpret_cast<uint64_t>(destination) & 0xFF80FFFFFFFFFFFF | 0x80000000000000;
        return;
    }

    this->data = (length & 0x7F) << 0x30 | reinterpret_cast<uint64_t>(data) & 0xFF00FFFFFFFFFFFF;
}

uint64_t Bedrock::StaticOptimizedString::length() const {
    const auto val = this->data;

    if ((val >> 0x37 & 1) == 0)
        return val >> 0x30 & 0xff;

    return *reinterpret_cast<uint64_t*>((val & 0xFF00FFFFFFFFFFFF) - 8);
}

Bedrock::StaticOptimizedString& Bedrock::StaticOptimizedString::operator=(const StaticOptimizedString& other) {
    if ((this->bytes[6] & 0x80) != 0)
        Memory::MemoryAllocator::alignedRelease(reinterpret_cast<void*>((this->data & 0xFF00FFFFFFFFFFFF) - 8));

    this->data = 0;

    const auto otherData = other.data;
    const auto source = otherData & 0xFF00FFFFFFFFFFFF;
    const auto sourceSize = static_cast<int8_t>(otherData >> 0x30);
    const auto size = sourceSize < 0 ? *reinterpret_cast<uint64_t*>(source - 0x8) : sourceSize & 0x7F;

    if (source == 0 || (sourceSize >= 0 && size < 0x80))
        this->data = (size & 0x7F) << 0x30 | otherData;
    else {
        const auto ptr = static_cast<uint64_t*>(Memory::MemoryAllocator::alignedAlloc(size + 9, 8));
        const auto destination = ptr + 1;

        *ptr = size;

        memcpy(destination, reinterpret_cast<const void*>(source), size);

        *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(destination) + size) = 0;

        this->data = reinterpret_cast<uint64_t>(destination) & 0xFF80FFFFFFFFFFFF | 0x80000000000000;
    }

    return *this;
}
