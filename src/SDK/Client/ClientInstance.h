#pragma once

#include "../Core/Minecraft.h"
#include "../Network/LoopbackPacketSender.h"
#include "../Render/Level/LevelRenderer.h"
#include "../World/Actor/LocalPlayer.h"
#include "../World/BlockSource.h"
#include "GUI/GuiData.h"
#include "MCE/Camera.h"

class ClientInstance {
public:
    class MinecraftGame* getMinecraftGame();
    Minecraft* getMinecraft();
    LevelRenderer* getLevelRenderer();
    LoopbackPacketSender* getPacketSender();
    mce::Camera& getCamera();
    GuiData* getGuiData();

    BlockSource* getBlockSource();
    LocalPlayer* getLocalPlayer();
};
