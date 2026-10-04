#include "Font.h"

#include "../../../Utils/Utils.h"

float Font::getLineLength(const std::string_view& str, const float fontSize, const bool showColorSymbol) {
    return Utils::CallVFunc<6, float, const std::string_view&, float, float>(this, str, fontSize, showColorSymbol);
}

float Font::getLineHeight() {
    return Utils::CallVFunc<7, float>(this);
}
