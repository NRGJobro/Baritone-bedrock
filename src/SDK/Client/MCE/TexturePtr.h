#pragma once

#include "../Resources/ResourceLocation.h"
#include "../Texture/BedrockTextureData.h"

namespace mce {
    class TexturePtr {
    public:
        std::shared_ptr<BedrockTextureData> clientTexture;
        std::shared_ptr<ResourceLocation> resourceLocationPtr;

        [[nodiscard]] const std::string& getTextureName() const {
            return resourceLocationPtr->path;
        }
    };
}
