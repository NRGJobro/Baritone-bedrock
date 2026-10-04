#include "MC.h"

#include "../Memory/Sig/SignatureManager.h"
#include "../Utils/Utils.h"
#include "Core/MainWindow.h"

static ClientInstance* clientInstance = nullptr;
static HWND windowHandle = nullptr;

void MC::init() {
    // 1.26.52 no longer exposes the old mMainWindow chain. The current
    // Platform_GameCore reference owns both the HWND and MinecraftGame.
    const auto referenceAddress = Utils::getFromOffset<void**>(
        GET_SIG("Platform_GameCore::instanceReference"), 3);
    if (referenceAddress == nullptr || *referenceAddress == nullptr)
        return;

    const auto reference = *referenceAddress;
    const auto platform = hat::member_at<void*>(reference, 0x8);
    if (platform == nullptr)
        return;

    windowHandle = hat::member_at<HWND>(platform, 0x80);
    const auto holder = hat::member_at<void*>(platform, 0x20);
    const auto game = holder == nullptr ? nullptr : hat::member_at<MinecraftGame*>(holder, 0x48);
    if (game != nullptr)
        clientInstance = game->getClientInstance();
}

HWND MC::getWindowHandle() {
    return windowHandle;
}

MinecraftGame* MC::getMinecraftGame() {
    if (clientInstance == nullptr)
        return nullptr;

    return clientInstance->getMinecraftGame();
}

ClientInstance* MC::getClientInstance() {
    return clientInstance;
}

LevelRenderer* MC::getLevelRenderer() {
    if (clientInstance == nullptr)
        return nullptr;

    return clientInstance->getLevelRenderer();
}

GuiData* MC::getGuiData() {
    if (clientInstance == nullptr)
        return nullptr;

    return clientInstance->getGuiData();
}

BlockSource* MC::getRegion() {
    if (clientInstance == nullptr)
        return nullptr;

    return clientInstance->getBlockSource();
}

LocalPlayer* MC::getLocalPlayer() {
    if (clientInstance == nullptr)
        return nullptr;

    return clientInstance->getLocalPlayer();
}
