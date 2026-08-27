#pragma once

#include "BlendTarget.h"

namespace mce {
    struct BlendStateDescription {
        BlendTarget blendSource;
        BlendTarget blendDestination;
        BlendTarget alphaSource;
        BlendTarget alphaDestination;
        uint8_t colorWriteMask;
        bool enableBlend;
        bool enableAlphaToCoverage;
    };
}
