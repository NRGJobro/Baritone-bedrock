#include "MC.h"

#include "../Memory/Sig/SignatureManager.h"
#include "../Utils/Utils.h"
#include "Core/MainWindow.h"

static ClientInstance* clientInstance = nullptr;
static HWND windowHandle = nullptr;

void MC::init() {
    const auto platform = hat::member_at<MainWindow*>(*Utils::getFromOffset<void**>(GET_SIG("mMainWindow"), 3), 0x8);

    clientInstance = platform->getMinecraftGame()->getClientInstance();
    windowHandle = platform->getHIDController()->getWindowHandle();
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
