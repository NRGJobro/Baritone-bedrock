#pragma once

#include "../Client/Font/Font.h"
#include "../Client/MCE/Color.h"
#include "CaretMeasureData.h"
#include "ScreenContext.h"
#include "TextMeasureData.h"

namespace mce { class TextureGroup; }
class ClientInstance;

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
