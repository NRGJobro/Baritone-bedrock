#pragma once

#include "Font.h"

class MSDFFont : public Font {
public:
    std::string fontPagePrefix; // 0x2B0
    std::unordered_map<int, float> unicodeWidths; // 0x2D0
    std::unordered_map<int, float> unicodeOffsets; // 0x310
    std::unordered_map<int, float> unicodePageGlyphWidths; // 0x350
};
