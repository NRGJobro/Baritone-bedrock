#include "Font.h"

#include "../../../Memory/Sig/SignatureManager.h"
#include "../../../Utils/Utils.h"
#include "../../Render/Resources/UIThumbnailMeshOffscreenCaptureDescription.h"

void Font::drawCached(ScreenContext* screenContext, const std::string_view& text, float x, float y, const mce::Color& color, bool unk, bool darken, bool drawColorSymbol, mce::MaterialPtr* optMaterial, int caretPosition, bool shadow, float linePadding, const mce::Color& resetColorOverride, const mce::Color& shadowColor, float shadowX, float shadowY) {
    Utils::CallVFunc<4, void, ScreenContext*, const std::string_view&, float, float, const mce::Color&, bool, bool, bool, mce::MaterialPtr*, int, bool, float, const mce::Color&, const mce::Color&, float, float, const std::variant<std::monostate, UIActorOffscreenCaptureDescription, UIThumbnailMeshOffscreenCaptureDescription, UIMeshOffscreenCaptureDescription, UIStructureVolumeCaptureDescription>&, bool>(this, screenContext, text, x, y, color, unk, darken, drawColorSymbol, optMaterial, caretPosition, shadow, linePadding, resetColorOverride, shadowColor, shadowX, shadowY, {}, false);
}

float Font::getLineLength(const std::string_view& str, const float fontSize, const bool showColorSymbol) {
    return Utils::CallVFunc<6, float, const std::string_view&, float, float>(this, str, fontSize, showColorSymbol);
}

float Font::getLineHeight() {
    return Utils::CallVFunc<7, float>(this);
}

float Font::getScaleFactor() {
    return Utils::CallVFunc<8, float>(this);
}

float Font::getScaleFactor(const int character) {
    return Utils::CallVFunc<9, float, int>(this, character);
}

float Font::_getCharWidth(const int character, const bool showColorSymbol) {
    return Utils::CallVFunc<28, float, int, bool>(this, character, showColorSymbol);
}

void Font::draw(ScreenContext* screenContext, const std::string& text, const float x, const float y, const mce::Color& color, const bool shadow, mce::MaterialPtr* optMaterial) {
    if (shadow)
        this->drawShadow(screenContext, text, x, y, color, false, optMaterial);
    else
        this->drawCached(screenContext, text, x, y, color, false, false, false, optMaterial);
}

void Font::drawShadow(ScreenContext* screenContext, const std::string_view& text, float x, float y, const mce::Color& color, bool drawColorSymbol, mce::MaterialPtr* optMaterial, float linePadding) {
    // The standalone helper was inlined in 1.26.52. Slot 4 is the stable
    // drawCached entry and accepts the shadow request explicitly.
    this->drawCached(screenContext, text, x, y, color, false, false,
        drawColorSymbol, optMaterial, -1, true, linePadding);
}
