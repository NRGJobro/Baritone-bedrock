#include "ClientInstance.h"

#include "../../Utils/Utils.h"

MinecraftGame* ClientInstance::getMinecraftGame() {
    return hat::member_at<MinecraftGame*>(this, 0x1A8);
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

GuiData* ClientInstance::getGuiData() {
    return hat::member_at<GuiData*>(this, 0x650);
}


BlockSource* ClientInstance::getBlockSource() {
    return Utils::CallVFunc<30, BlockSource*>(this);
}

LocalPlayer* ClientInstance::getLocalPlayer() {
    return Utils::CallVFunc<31, LocalPlayer*>(this);
}

void ClientInstance::grabMouse() {
    Utils::CallVFunc<310, void>(this);
}

void ClientInstance::releaseMouse() {
    Utils::CallVFunc<311, void>(this);
}
