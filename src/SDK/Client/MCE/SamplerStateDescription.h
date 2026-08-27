#pragma once

#include "ComparisonFunc.h"
#include "TextureFiltering.h"
#include "TextureWrappingDescription.h"

namespace mce {
    struct SamplerStateDescription {
        TextureFiltering textureFilter;
        TextureWrappingDescription textureWrappingDescription;
        uint32_t shaderStagesBits;
        int16_t samplerIndex;
        ComparisonFunc comparisonFunc;
    };
}
