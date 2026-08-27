#pragma once

#include "../../Bedrock/NonOwnerPointer.h"
#include "FontRepository.h"

struct FontHandle : Bedrock::EnableNonOwnerReferences {
    Bedrock::NonOwnerPointerRef<FontRepository> fontRepository;
    std::shared_ptr<Font> defaultFont;
    uint64_t fontId;
    bool isDummyHandle;
};
