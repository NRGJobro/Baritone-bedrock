#include "TextureUtils.h"

#include "../SDK/Client/MinecraftGame.h"
#include "../SDK/Client/cg/ImageBuffer.h"
#include "../SDK/MC.h"

mce::ClientTexture& TextureUtils::uploadImageData(const std::vector<mce::Color>& colors, const uint32_t width, const uint32_t height, const std::string& uploadName) {
    const auto imageData = std::make_unique<uint8_t[]>(width * height * 4);

    for (int i = 0; i < width * height; i++) {
        const auto& color = colors[i];

        imageData[i * 4] = static_cast<uint8_t>(color.r * 255.f) & 0xFF;
        imageData[i * 4 + 1] = static_cast<uint8_t>(color.g * 255.f) & 0xFF;
        imageData[i * 4 + 2] = static_cast<uint8_t>(color.b * 255.f) & 0xFF;
        imageData[i * 4 + 3] = static_cast<uint8_t>(color.a * 255.f) & 0xFF;
    }

    return uploadImageData(imageData.get(), width, height, uploadName);
}

mce::ClientTexture& TextureUtils::uploadImageData(uint8_t* colors, const uint32_t width, const uint32_t height, const std::string& uploadName) {
    cg::ImageBuffer buffer;

    buffer.storage = {colors, static_cast<size_t>(width * height * 4)};
    buffer.imageDescription = {width, height, mce::TextureFormat::R8g8b8a8Unorm, cg::ColorSpace::sRGB};

    const ResourceLocation location{uploadName, ResourceFileSystem::UserPackage};

    return MC::getMinecraftGame()->getTextureGroup()->uploadTexture(location, buffer).texture->clientTexture;
}
