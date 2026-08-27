#pragma once

#include "ITransactionContainer.h"

namespace mce {
    template<typename T>
    class SimpleDeferredResourceTracker {
    public:
        std::vector<std::pair<std::weak_ptr<T>, std::unique_ptr<mce::ITransactionContainer>>> pendingTransactionHandles;
    };
}
