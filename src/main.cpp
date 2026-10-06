#include "Client.h"
#include "Client/Modules/ModuleManager.h"
#include "Memory/Hook/HookManager.h"
#include "Memory/Sig/SigInit.h"
#include "Memory/Sig/SignatureManager.h"
#include "SDK/MC.h"
#include "Utils/Logger.h"
#include "Utils/Utils.h"

DWORD WINAPI start(LPVOID module) {
    Utils::createFolder(Utils::getClientFolder());
    Logger::initializeLogger();
    Logger::clearLogs();

    MH_Initialize();
    SigInit::addSigs();
    sigMgr.scanAll();

    const std::array requiredSignatures{
        std::pair{"mce::Mesh::_renderMesh", GET_SIG("mce::Mesh::_renderMesh")},
        std::pair{"mce::RenderMaterialGroup::common", GET_SIG("mce::RenderMaterialGroup::common")},
        std::pair{"RenderContextHook::ctxSig", GET_SIG("RenderContextHook::ctxSig")},
        std::pair{"WindowProcCallbackHook::keymapSig", GET_SIG("WindowProcCallbackHook::keymapSig")},
        std::pair{"UpdateHook::updateSig", GET_SIG("UpdateHook::updateSig")},
        std::pair{"LevelRendererHook::levelRendererHookSig", GET_SIG("LevelRendererHook::levelRendererHookSig")},
        std::pair{"GammaHook::gammaSig", GET_SIG("GammaHook::gammaSig")},
        std::pair{"Platform_GameCore::instanceReference", GET_SIG("Platform_GameCore::instanceReference")},
        std::pair{"GuiData::displayClientMessage", GET_SIG("GuiData::displayClientMessage")},
        std::pair{"MinecraftPackets::createPacket", GET_SIG("MinecraftPackets::createPacket")},
    };
    for (const auto& [name, address] : requiredSignatures) {
        if (address == 0) {
            logF("Limiter startup aborted: required signature '{}' was not found", name);
            MH_Uninitialize();
            FreeLibraryAndExitThread(static_cast<HMODULE>(module), 1);
        }
    }

    while (g_Client.running && (MC::getClientInstance() == nullptr || MC::getWindowHandle() == nullptr)) {
        MC::init();
        Sleep(25);
    }
    if (!g_Client.running) {
        MH_Uninitialize();
        FreeLibraryAndExitThread(static_cast<HMODULE>(module), 0);
    }

    if (IsWindow(MC::getWindowHandle()))
        SetWindowTextA(MC::getWindowHandle(), "Limiter - Minecraft");

    g_modMgr.init();
    HookManager::initializeHooks();
    HookManager::setHooksEnabled(true);
    logF("Limiter initialized");

    while (g_Client.running)
        Sleep(10);

    g_modMgr.shutdown();
    HookManager::setHooksEnabled(false);
    HookManager::destroy();
    MH_Uninitialize();

    if (IsWindow(MC::getWindowHandle()))
        SetWindowTextA(MC::getWindowHandle(), "Minecraft");
    FreeLibraryAndExitThread(static_cast<HMODULE>(module), 0);
}

BOOL WINAPI DllMain(HINSTANCE instance, const DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        CreateThread(nullptr, 0, start, instance, 0, nullptr);
    }
    return TRUE;
}
