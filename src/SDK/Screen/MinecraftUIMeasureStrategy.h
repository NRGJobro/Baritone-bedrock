#pragma once

#include "UIProfanityContext.h"

struct MinecraftUIMeasureStrategy {
    void** vtable;
    void** uiProfanityContextVtable;
    std::shared_ptr<UIProfanityContext> uiProfanityContext;
};
