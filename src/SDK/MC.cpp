#include "MC.h"

#include "../Memory/Sig/SignatureManager.h"
#include "../Utils/Utils.h"

static std::atomic<ClientInstance*> clientInstance{nullptr};
static std::atomic<HWND> windowHandle{nullptr};

void MC::init() {
    // 1.26.52 no longer exposes the old mMainWindow chain. The current
    // Platform_GameCore reference owns both the HWND and MinecraftGame.
    const auto referenceAddress = Utils::getFromOffset<void**>(
        GET_SIG("Platform_GameCore::instanceReference"), 3);
    if (referenceAddress == nullptr || *referenceAddress == nullptr) {
        clientInstance.store(nullptr, std::memory_order_release);
        return;
    }

    const auto reference = *referenceAddress;
    const auto platform = hat::member_at<void*>(reference, 0x8);
    if (platform == nullptr) {
        clientInstance.store(nullptr, std::memory_order_release);
        return;
    }

    windowHandle.store(hat::member_at<HWND>(platform, 0x80), std::memory_order_release);
    const auto holder = hat::member_at<void*>(platform, 0x20);
    const auto game = holder == nullptr ? nullptr : hat::member_at<MinecraftGame*>(holder, 0x48);
    clientInstance.store(game == nullptr ? nullptr : game->getClientInstance(),
        std::memory_order_release);
}

void MC::setClientInstance(ClientInstance* instance) {
    clientInstance.store(instance, std::memory_order_release);
}

void MC::reset() {
    clientInstance.store(nullptr, std::memory_order_release);
    windowHandle.store(nullptr, std::memory_order_release);
}

HWND MC::getWindowHandle() {
    return windowHandle.load(std::memory_order_acquire);
}

MinecraftGame* MC::getMinecraftGame() {
    const auto instance = clientInstance.load(std::memory_order_acquire);
    if (instance == nullptr)
        return nullptr;

    return instance->getMinecraftGame();
}

ClientInstance* MC::getClientInstance() {
    return clientInstance.load(std::memory_order_acquire);
}

LevelRenderer* MC::getLevelRenderer() {
    const auto instance = clientInstance.load(std::memory_order_acquire);
    if (instance == nullptr)
        return nullptr;

    return instance->getLevelRenderer();
}

GuiData* MC::getGuiData() {
    const auto instance = clientInstance.load(std::memory_order_acquire);
    if (instance == nullptr)
        return nullptr;

    return instance->getGuiData();
}

BlockSource* MC::getRegion() {
    const auto instance = clientInstance.load(std::memory_order_acquire);
    if (instance == nullptr)
        return nullptr;

    return instance->getBlockSource();
}

LocalPlayer* MC::getLocalPlayer() {
    const auto instance = clientInstance.load(std::memory_order_acquire);
    if (instance == nullptr)
        return nullptr;

    return instance->getLocalPlayer();
}
