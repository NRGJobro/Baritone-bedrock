#pragma once

class Font;
class ScreenContext;
class ClientInstance;
struct TextMeasureData;
struct CaretMeasureData;
namespace mce { class Color; class TextureGroup; }

class MinecraftUIRenderContext {
    void** vtable;

public:
    ClientInstance* clientInstance;
    ScreenContext* screenContext;
    std::byte reserved[0x40];
    std::shared_ptr<mce::TextureGroup> textureGroup;

    void drawText(Font* font, const glm::vec4& pos, const std::string& text, const mce::Color& color, float alpha, float textAlignment,
        const TextMeasureData& textData, const CaretMeasureData& caretData);
    void flushText();
};
