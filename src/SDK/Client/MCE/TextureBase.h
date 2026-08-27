#pragma once

#include "TextureDescription.h"

namespace mce {
    struct TextureBase {
        TextureDescription textureDescription;
        bool created;
        bool ownsResource;
    };
}
