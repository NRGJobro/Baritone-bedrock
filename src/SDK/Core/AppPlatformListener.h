#pragma once

class AppPlatformListener {
    void** vtable;
    std::shared_ptr<__int64> lowMemorySubscription;
    bool listenerRegistered{};
};