#pragma once

#include "../../Bedrock/EnableNonOwnerReferences.h"
#include "../../Core/AppPlatformListener.h"
#include "Font.h"

class FontRepository : public AppPlatformListener, public Bedrock::EnableNonOwnerReferences {
public:
    bool isInitialized;
    std::vector<std::shared_ptr<Font>> loadedFonts;
    std::unordered_map<std::string, uint64_t> fontNameToIdentifier;

    uint64_t getFontIdentifier(const std::string& fontName);
};
