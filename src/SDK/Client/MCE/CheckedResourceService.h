#pragma once

#include "BufferResourceServiceContext.h"
#include "ResourceBlockTemplate.h"
#include "ResourceServiceState.h"
#include "SimpleResourceTracker.h"

namespace mce {
    template<typename T>
    class CheckedResourceService {
        SimpleResourceTracker<ResourceBlockTemplate<T>> resourceTracker;
        BufferResourceServiceContext resourceServiceContext;
        ResourceServiceState state;
    };
}
