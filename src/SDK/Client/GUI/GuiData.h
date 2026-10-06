#pragma once

#include "ScreenSizeData.h"

struct GuiData {
    std::byte reserved[0x40];

    // Phase 1.26.52: physical resolution begins at 0x40, rounded/client
    // resolution at 0x48, and UI resolution at 0x50.
    ScreenSizeData screenSizeData;

    template <typename... Args>
    void displayClientMessageF(const std::string& text, Args... args) {
        displayClientMessage(fmt::vformat(text, fmt::make_format_args(args...)));
    }

    void displayClientMessage(const std::string& message);
};

static_assert(offsetof(GuiData, screenSizeData) == 0x40);
