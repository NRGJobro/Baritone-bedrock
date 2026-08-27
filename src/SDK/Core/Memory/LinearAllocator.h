#pragma once

#include "AllocatorData.h"

template <typename T>
class LinearAllocator {
public:
    using value_type = T;

    std::shared_ptr<AllocatorData> data;
};
