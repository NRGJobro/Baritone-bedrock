#pragma once

#include "BlitFlipbookSingleTextureDescription.h"

namespace mce::framebuilder {
    struct BlitFlipbookTextureDescription {
        dragon::res::ServerTexture renderTargetTexture;
        std::span<BlitFlipbookSingleTextureDescription> textures;
    };
}
