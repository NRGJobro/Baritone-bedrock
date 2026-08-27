#pragma once

#include "RenderContextBase.h"

namespace mce {
    struct RenderContext : RenderContextBase {
        bool withinFrame;
    };
}
