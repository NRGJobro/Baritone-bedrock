#pragma once

#include "../../Bedrock/EnableNonOwnerReferences.h"
#include "../Texture/BedrockTexture.h"
#include "../Texture/UITextureInfo.h"
#include "../cg/ImageBuffer.h"
#include "LRUCache.h"
#include "TextureContainer.h"
#include "TextureGroupBase.h"

namespace mce {
    class TextureGroup : public Bedrock::EnableNonOwnerReferences, public TextureGroupBase {
    public:
        LRUCache* getLRUCache();

        BedrockTexture& uploadTexture(const ResourceLocation& location, cg::ImageBuffer& buffer);
        BedrockTexture& uploadTexture2(const ResourceLocation& location, TextureContainer& container, std::optional<std::string_view> debugName);

        void unloadTexture(const ResourceLocation& location);
        bool isLoaded(const ResourceLocation& location);
        IsMissingTexture isMissingTexture(const ResourceLocation& location);

        std::map<ResourceLocation, BedrockTexture>& getTextures();
        std::map<ResourceLocation, UITextureInfo>& getUITextures();

        BedrockTexture* getTextureRef(const ResourceLocation& location);

        TextureGroup* toTextureGroupBase() {
            return reinterpret_cast<TextureGroup*>(reinterpret_cast<uintptr_t>(this) + 0x18);
        }
    };
}
