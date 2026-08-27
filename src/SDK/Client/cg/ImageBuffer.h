#pragma once

#include "ImageDescription.h"
#include "../mce/Blob.h"
#include "../mce/Image.h"

namespace cg {
    struct ImageBuffer {
        mce::Blob storage;
        ImageDescription imageDescription;

        ImageBuffer() = default;
        explicit ImageBuffer(const mce::Image& image);

        bool isValid() const;
    };
}
