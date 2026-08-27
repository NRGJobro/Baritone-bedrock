#pragma once

#include "../MCE/Color.h"
#include "../MCE/Mesh.h"
#include "../MCE/TextureGroup.h"

class Font : public std::enable_shared_from_this<Font> {
public:
    struct SheetId {
        const void* rawFontPtr;
        int index;
    };

    struct TextObject {
        struct Page {
            mce::Mesh mesh;
            mce::TexturePtr texture;
            bool renderSmooth;
            SheetId sheet;
        };

        std::vector<Page> pages;
        mce::Color color;
        bool containsUnicode;
        bool shadow;
        mce::Color shadowColor;
        glm::vec2 shadowOffset;
    };

private:
    char pad0x0[0x8]; // "0x0"; the vtable

public:
    mce::Color colors[32]; // 0x18
    int fontTexture; // 0x218

private:
    char pad0x220[0x18]; // 0x220

public:
    std::shared_ptr<mce::TextureGroup> textureGroup; // 0x238
    std::map<std::tuple<std::string, mce::Color, float>, std::vector<TextObject>> stringCache; // 0x248
    int obfuscatedIndex; // 0x258
    float obfuscatedTextTime; // 0x25C
    glm::vec2 caretRenderPosition; //0x260
    glm::vec2 caretRenderSize; //0x268
    std::bitset<8> formatBits; //0x270
    std::optional<mce::Color> currentColor; // 0x274
    mce::Color shadowColor; // 0x288
    bool italic; // 0x298
    bool bold; // 0x299
    bool strikethrough; // 0x29A
    bool underlined; // 0x29B
    bool obfuscated; // 0x29C
    mce::MaterialPtr fontMat; // 0x2A0

    void drawCached(class ScreenContext* screenContext, const std::string_view& text, float x, float y, const mce::Color& color = {}, bool unk = false, bool darken = false, bool drawColorSymbol = false, mce::MaterialPtr* optMaterial = nullptr, int caretPosition = -1, bool shadow = false, float linePadding = 0.f, const mce::Color& resetColorOverride = {0.f, 0.f, 0.f, 0.f}, const mce::Color& shadowColor = {0.f, 0.f, 0.f, 0.f}, float shadowX = 0.f, float shadowY = 0.f);
    float getLineLength(const std::string_view& str, float fontSize, bool showColorSymbol = false);
    float getLineHeight();
    float getScaleFactor();
    float getScaleFactor(int character);
    float _getCharWidth(int character, bool showColorSymbol = false);

    void draw(ScreenContext* screenContext, const std::string& text, float x, float y, const mce::Color& color = {}, bool shadow = false, mce::MaterialPtr* optMaterial = nullptr);
    void drawShadow(ScreenContext* screenContext, const std::string_view& text, float x, float y, const mce::Color& color = {}, bool drawColorSymbol = false, mce::MaterialPtr* optMaterial = nullptr, float linePadding = 0.f);
};
