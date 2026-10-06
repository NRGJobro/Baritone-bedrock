#pragma once

#include "Client/MinecraftGame.h"

namespace MC {
    void init();
    void setClientInstance(ClientInstance* instance);
    void reset();

    HWND getWindowHandle();
    MinecraftGame* getMinecraftGame();
    ClientInstance* getClientInstance();
    LevelRenderer* getLevelRenderer();
    GuiData* getGuiData();
    BlockSource* getRegion();
    LocalPlayer* getLocalPlayer();
}
