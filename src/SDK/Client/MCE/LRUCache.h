#pragma once

#include "../Resources/ResourceLocation.h"

namespace mce {
    class LRUCache {
    public:
        void remove(const ResourceLocation& location);
    };
}
