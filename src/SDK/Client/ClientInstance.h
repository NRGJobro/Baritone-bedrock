#pragma once

#include "../Core/Minecraft.h"
#include "../Network/LoopbackPacketSender.h"
#include "../Render/Level/LevelRenderer.h"
#include "../Render/LightTexture.h"
#include "../World/Actor/LocalPlayer.h"
#include "../World/BlockSource.h"
#include "GUI/GuiData.h"
#include "MCE/Camera.h"
#include "Social/User.h"

class ClientInstance {
public:
    class MinecraftGame* getMinecraftGame();
    Minecraft* getMinecraft();
    LevelRenderer* getLevelRenderer();
    LoopbackPacketSender* getPacketSender();
    mce::Camera& getCamera();
    glm::vec2& getMousePos();
    GuiData* getGuiData();
    Social::User* getUser();
    void* getOptions();

    BlockSource* getBlockSource();
    LocalPlayer* getLocalPlayer();
    LightTexture* getLightTexture();
};
