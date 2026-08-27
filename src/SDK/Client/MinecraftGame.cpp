#include "MinecraftGame.h"

#include "../../Utils/Utils.h"

ClientInstance* MinecraftGame::getClientInstance() {
    return hat::member_at<std::map<uint8_t, std::shared_ptr<ClientInstance>>>(this, 0xA08)[0].get();
}

std::shared_ptr<mce::TextureGroup> MinecraftGame::getTextureGroup() {
    return hat::member_at<std::shared_ptr<mce::TextureGroup>>(this, 0x7A0);
}

std::shared_ptr<FontRepository> MinecraftGame::getFontRepository() {
    return hat::member_at<std::shared_ptr<FontRepository>>(this, 0x1B0);
}

ServerInstance* MinecraftGame::getServerInstance() {
    return hat::member_at<ServerInstance*>(this, 0x10F8);
}

void MinecraftGame::grabMouse() {
    Utils::CallVFunc<144, void>(this);
}

void MinecraftGame::releaseMouse() {
    Utils::CallVFunc<145, void>(this);
}

Font* MinecraftGame::getFont(const Fonts font) {
    static bool hasCachedAllFonts = false;
    static std::unordered_map<Fonts, Font*> cachedFonts;
    const auto repo = this->getFontRepository();

    if (!hasCachedAllFonts) {
        magic_enum::enum_for_each<Fonts>([&](auto loopedFont) {
            Fonts currentFont = loopedFont;
            cachedFonts.emplace(currentFont, repo->loadedFonts.at(repo->getFontIdentifier(std::string(magic_enum::enum_name(currentFont)))).get());
        });
        hasCachedAllFonts = true;
    }

    return cachedFonts[font];
}
