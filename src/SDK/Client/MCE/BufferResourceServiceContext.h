#pragma once

#include "ResourceServiceRenderContext.h"

namespace mce {
    class BufferResourceServiceContext : public ResourceServiceRenderContext {
    public:
        bool frameBuilderEnabled;
    };
}
