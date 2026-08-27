#include "MinecraftUIRenderContext.h"

#include "../../Utils/Utils.h"

void MinecraftUIRenderContext::drawText(Font* font, const glm::vec4& pos, const std::string& text, const mce::Color& color, const float alpha,
    const ui::TextAlignment textAlignment, const TextMeasureData& textData, const CaretMeasureData& caretData) {
    Utils::CallVFunc<5, void, Font*, const glm::vec4&, const std::string&, const mce::Color&, float, ui::TextAlignment, const TextMeasureData&, const CaretMeasureData&>(this, font, pos, text, color, alpha, textAlignment, textData, caretData);
}

void MinecraftUIRenderContext::flushText() {
    Utils::CallVFunc<6, void, float, std::optional<float>>(this, 1.f, {});
}
