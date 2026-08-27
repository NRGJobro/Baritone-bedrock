#include "FontRepository.h"

uint64_t FontRepository::getFontIdentifier(const std::string& fontName) {
    return fontNameToIdentifier[fontName];
}
