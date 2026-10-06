#pragma once

class MinecraftGame;
class ClientInstance;
class LevelRenderer;
class GuiData;
class BlockSource;
class LocalPlayer;

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
