#pragma once

#include "../../Bedrock/PrioritizeDefault.h"
#include "../MCE/TextureGroup.h"
#include "Font.h"

class TrueTypeFont : public Font {
    char pad0x2B0[0x80]{}; // 0x2B0 Two unordered maps of unknown types

public:
    struct LoadedFontInformation {
        stbtt_fontinfo info; // 0x0 (0x330)
        float scale; // 0xA0 (0x3D0)
        int lineGap; // 0xA4 (0x3D4)
        int descent; // 0xA8 (0x3D8)
        int ascent; // 0xAC (0x3DC)
        std::string resourceData; // 0xB0 (0x3E0)
        std::string path; // 0xD0 (0x400)
    };

    LoadedFontInformation loadedFontInformation; // 0x330
    uint32_t version; // 0x420
    uint8_t defaultRenderSize; // 0x424
    uint16_t atlasPageSize; // 0x426
    float renderSizeToGameSizeScalar; // 0x428
    std::vector<std::pair<ResourceLocation, cg::ImageBuffer>> texturesToUpload; // 0x430
    bool reloading; // 0x448
    Bedrock::Threading::PrioritizeDefault* reloadMutex; // 0x450
    bool loaded; // 0x458

    bool isLoaded() const;
};
