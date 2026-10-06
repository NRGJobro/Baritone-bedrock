#pragma once

#include "Blob.h"
#include "ImageFormat.h"
#include "ImageUsage.h"

namespace mce {
    struct Image {
        ImageFormat imageFormat;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        ImageUsage usage;
        Blob imageData;

        Image() = default;
        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;
    };
}
