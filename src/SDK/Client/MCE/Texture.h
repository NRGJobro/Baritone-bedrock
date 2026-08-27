#pragma once

#include "DragonLifetime.h"
#include "ResourceBase.h"
#include "TextureNull.h"

namespace mce {
    struct Texture : TextureNull, ResourceBase<TextureNull> {
        std::shared_ptr<DragonLifetime> dragonTextureLifetime;
        std::optional<uint64_t> renderDragonTexture;
    };
}
