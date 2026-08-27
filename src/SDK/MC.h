#pragma once

#include "Client/MinecraftGame.h"

namespace MC {
    void init();

    HWND getWindowHandle();
    MinecraftGame* getMinecraftGame();
    ClientInstance* getClientInstance();
    LevelRenderer* getLevelRenderer();
    GuiData* getGuiData();
    BlockSource* getRegion();
    LocalPlayer* getLocalPlayer();
}
