#pragma once

#include "../Client/Font/Font.h"
#include "../Client/Font/FontHandle.h"
#include "../Client/MCE/TextureGroup.h"
#include "../Client/ui/TextAlignment.h"
#include "../Screen/MinecraftUIMeasureStrategy.h"
#include "../Screen/UIRepository.h"
#include "../Screen/UIScene.h"
#include "CaretMeasureData.h"
#include "ScreenContext.h"
#include "TextMeasureData.h"

class MinecraftUIRenderContext {
    void** vtable;

public:
    struct TextItem {
        Font* font;
        RectangleArea area;
        std::string text;
        mce::Color color;
        bool shadow;
        bool showColorSymbol;
        int caretLocation;
        float fontScale;
        float linePadding;
        ui::TextAlignment alignment;

        TextItem() = default;
        TextItem(Font* font, const RectangleArea& area, const std::string& text, const mce::Color& color, const float alpha, const bool shadow, const bool showColorSymbol,
            const int caretLocation, const float fontScale, const float linePadding, const ui::TextAlignment alignment) :
            font(font), area(area), text(text), color(color.r, color.g, color.b, alpha), shadow(shadow), showColorSymbol(showColorSymbol),
            caretLocation(caretLocation), fontScale(fontScale), linePadding(linePadding), alignment(alignment) {}
    };

    struct ImageItem {
        mce::ClientTexture* texture;
        glm::vec2 pos;
        glm::vec2 size;
        glm::vec2 uv;
        glm::vec2 uvSize;
        bool unknown;
    };

    ClientInstance* clientInstance;
    ScreenContext* screenContext;
    MinecraftUIMeasureStrategy measureStrategy;
    float textAlpha;
    Bedrock::NonOwnerPointerRef<UIRepository> uiRepository;
    std::shared_ptr<mce::TextureGroup> textures;
    Bedrock::NonOwnerPointerRef<mce::TextureGroup> storeCacheTextures;
    std::vector<TextItem> textToDraw;
    std::vector<ImageItem> imagesToDraw;
    std::vector<std::unique_ptr<int64_t>> persistentMeshes;
    uint8_t stencilRef;
    int currentPersistentMeshItemIdx;
    FontHandle debugTextFontHandle;
    UIScene* currentScene;
    std::optional<glm::vec4> savedOriginalClippingRectangle;

    void drawText(Font* font, const glm::vec4& pos, const std::string& text, const mce::Color& color, float alpha, ui::TextAlignment textAlignment,
        const TextMeasureData& textData, const CaretMeasureData& caretData);
    void flushText();
};
