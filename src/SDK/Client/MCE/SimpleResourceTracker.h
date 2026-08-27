#pragma once

#include "SimpleDeferredResourceTracker.h"

namespace mce {
    template<typename T>
    class SimpleResourceTracker {
    public:
        std::vector<std::weak_ptr<T>> resourceTrackingBlocks;
        SimpleDeferredResourceTracker<std::shared_ptr<T>> deferredResourceTracker;
    };
}
