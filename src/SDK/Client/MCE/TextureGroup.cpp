#include "TextureGroup.h"

#include "../../../Memory/Sig/SignatureManager.h"

mce::LRUCache* mce::TextureGroup::getLRUCache() {
    return hat::member_at<LRUCache*>(this, 0x240);
}

BedrockTexture &mce::TextureGroup::uploadTexture(const ResourceLocation &location, cg::ImageBuffer &buffer) {
    TextureContainer container(buffer);
    return uploadTexture2(location, container, {});
}

BedrockTexture& mce::TextureGroup::uploadTexture2(const ResourceLocation& location, TextureContainer& container, std::optional<std::string_view> debugName) {
    static auto sig = GET_SIG("mce::TextureGroup::uploadTexture");
    using func_t = BedrockTexture&(__fastcall*)(TextureGroup*, const ResourceLocation&, TextureContainer&, std::optional<std::string_view>);
    static auto func = reinterpret_cast<func_t>(sig);
    return func(this, location, container, debugName);
}

void mce::TextureGroup::unloadTexture(const ResourceLocation& location) {
    if (this->getLRUCache() != nullptr)
        this->getLRUCache()->remove(location);

    auto& textures = this->getTextures();

    if (textures.contains(location)) {
        auto& texture = textures.at(location);

        texture.unload();

        textures.erase(location);

        auto& uiTextures = this->getUITextures();

        if (const auto it = uiTextures.find(location); it != uiTextures.end())
            uiTextures.erase(it);
    }
}

bool mce::TextureGroup::isLoaded(const ResourceLocation& location) {
    if (!this->getTextures().contains(location))
        return false;

    const auto& texture = this->getTextures().at(location);

    return texture.texture->clientTexture.isValid();
}

IsMissingTexture mce::TextureGroup::isMissingTexture(const ResourceLocation& location) {
    if (!this->getTextures().contains(location))
        return IsMissingTexture::Yes;

    const auto& tex = this->getTextures().at(location);

    return tex.texture->isMissingTexture;
}

std::map<ResourceLocation, BedrockTexture>& mce::TextureGroup::getTextures() {
    return hat::member_at<std::map<ResourceLocation, BedrockTexture>>(this, 0x1A8);
}

std::map<ResourceLocation, UITextureInfo>& mce::TextureGroup::getUITextures() {
    return hat::member_at<std::map<ResourceLocation, UITextureInfo>>(this, 0x230);
}

BedrockTexture* mce::TextureGroup::getTextureRef(const ResourceLocation& location) {
    if (!this->getTextures().contains(location)) {
        for (auto& [loc, tex] : this->getTextures()) {
            if (loc.path == location.path)
                return &tex;
        }

        return nullptr;
    }

    return &this->getTextures().at(location);
}
