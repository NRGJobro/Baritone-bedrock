#pragma once

struct SpinLockImpl {
    std::hash<std::thread::id> threadHasher;
    const uint64_t noThreadId = 0;
    std::atomic<uint64_t> ownerThread = 0;
    uint32_t ownerRefCount = 0;
};
