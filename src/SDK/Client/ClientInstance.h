#pragma once

class MinecraftGame;
class LevelRenderer;
class LoopbackPacketSender;
class GuiData;
class BlockSource;
class LocalPlayer;
namespace mce { class Camera; }

class ClientInstance {
public:
    class MinecraftGame* getMinecraftGame();
    LevelRenderer* getLevelRenderer();
    LoopbackPacketSender* getPacketSender();
    mce::Camera& getCamera();
    GuiData* getGuiData();

    BlockSource* getBlockSource();
    LocalPlayer* getLocalPlayer();
    void grabMouse();
    void releaseMouse();
};
