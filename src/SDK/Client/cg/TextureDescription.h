#pragma once

#include "ImageDescription.h"

namespace cg {
    struct TextureDescription : ImageDescription {
        uint32_t mipCount = 0;

        TextureDescription() = default;
        explicit TextureDescription(const mce::Image& image) : ImageDescription(image), mipCount(1) { }
        explicit TextureDescription(const ImageDescription& desc) : ImageDescription(desc), mipCount(1) { }
    };
}
