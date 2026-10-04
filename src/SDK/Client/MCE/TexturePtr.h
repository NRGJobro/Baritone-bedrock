#pragma once

class BedrockTextureData;
class ResourceLocation;

namespace mce {
    class TexturePtr {
    public:
        std::shared_ptr<BedrockTextureData> clientTexture;
        std::shared_ptr<ResourceLocation> resourceLocationPtr;
    };
}
