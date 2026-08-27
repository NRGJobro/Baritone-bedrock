#pragma once

#include "../cg/ImageResource.h"
#include "TextureDescription.h"

namespace mce {
    struct TextureContainer {
        std::shared_ptr<cg::ImageResource> storage{};
        TextureDescription description{};
        bool valid;

        TextureContainer() = default;
        explicit TextureContainer(Image& image);
        explicit TextureContainer(cg::ImageBuffer& image);
    };
}
