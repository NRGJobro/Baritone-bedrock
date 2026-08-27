#pragma once

#include "TextureHandle.h"

namespace bgfx {
    struct Attachment {
        TextureHandle handle;
        uint16_t mip;
        uint16_t layer;
    };
}
