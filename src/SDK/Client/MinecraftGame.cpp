#include "MinecraftGame.h"

#include "ClientInstance.h"
#include "Font/Font.h"
#include "Font/FontRepository.h"
#include "Font/Fonts.h"
#include "../../Utils/Utils.h"

ClientInstance* MinecraftGame::getClientInstance() {
    auto& instances = hat::member_at<std::map<uint8_t, CIHolder>>(this, 0x970);
    const auto primary = instances.find(0);
    return primary == instances.end() ? nullptr : primary->second.clientInstance.get();
}

FontRepository* MinecraftGame::getFontRepository() {
    return hat::member_at<FontRepository*>(this, 0x730);
}

Font* MinecraftGame::getFont(const Fonts font) {
    static bool hasCachedAllFonts = false;
    static std::unordered_map<Fonts, Font*> cachedFonts;
    const auto repo = this->getFontRepository();
    if (repo == nullptr || repo->loadedFonts.empty())
        return nullptr;

    if (!hasCachedAllFonts) {
        magic_enum::enum_for_each<Fonts>([&](auto loopedFont) {
            Fonts currentFont = loopedFont;
            const auto name = std::string(magic_enum::enum_name(currentFont));
            const auto identifier = repo->fontNameToIdentifier.find(name);
            const auto index = identifier == repo->fontNameToIdentifier.end() ? 0 : identifier->second;
            cachedFonts.emplace(currentFont,
                repo->loadedFonts.at(std::min<std::size_t>(index, repo->loadedFonts.size() - 1)).get());
        });
        hasCachedAllFonts = true;
    }

    return cachedFonts[font];
}
