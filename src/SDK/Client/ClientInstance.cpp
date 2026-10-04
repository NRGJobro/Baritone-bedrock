#include "ClientInstance.h"

#include "../../Utils/Utils.h"

MinecraftGame* ClientInstance::getMinecraftGame() {
    return hat::member_at<MinecraftGame*>(this, 0x1A8);
}

Minecraft* ClientInstance::getMinecraft() {
    return hat::member_at<Minecraft*>(this, 0x1B0);
}

LevelRenderer* ClientInstance::getLevelRenderer() {
    return hat::member_at<LevelRenderer*>(this, 0x1C0);
}

LoopbackPacketSender* ClientInstance::getPacketSender() {
    return hat::member_at<LoopbackPacketSender*>(this, 0x1D0);
}

mce::Camera& ClientInstance::getCamera() {
    return hat::member_at<mce::Camera>(this, 0x360);
}

glm::vec2& ClientInstance::getMousePos() {
    return hat::member_at<glm::vec2>(this, 0x580);
}

GuiData* ClientInstance::getGuiData() {
    return hat::member_at<GuiData*>(this, 0x650);
}

Social::User* ClientInstance::getUser() {
    return hat::member_at<Social::User*>(this, 0xBF8);
}

void* ClientInstance::getOptions() {
    return hat::member_at<void*>(this, 0xE10);
}


BlockSource* ClientInstance::getBlockSource() {
    return Utils::CallVFunc<30, BlockSource*>(this);
}

LocalPlayer* ClientInstance::getLocalPlayer() {
    return Utils::CallVFunc<31, LocalPlayer*>(this);
}

LightTexture* ClientInstance::getLightTexture() {
    return Utils::CallVFunc<192, LightTexture*>(this);
}
