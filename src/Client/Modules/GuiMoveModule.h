#pragma once

#include "Module.h"

class GuiMoveModule final : public Module {
public:
    GuiMoveModule();

    std::string getName() override;
    void onTick() override;
};
