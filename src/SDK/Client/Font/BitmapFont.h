#pragma once

#include "../mce/Image.h"
#include "Font.h"

class BitmapFont : public Font {
public:
    std::string asciiFontName; // 0x2B0
    std::string unicodeFontName; // 0x2D0
    float charWidths[0x100]; // 0x2F0
    std::shared_ptr<mce::Image> bitmapFontImage; // 0x6F0
    std::unordered_map<int, float> unicodeWidths; // 0x700
    std::unordered_map<int, float> unicodeOffsets; // 0x740
    std::unordered_map<int, float> unicodePageGlyphWidths; // 0x780
    std::unordered_set<int> sheetScannedForWidthsAndOffsets; // 0x7C0
};
