#include "Client.h"
#include "Client/Module/ModuleManager.h"
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
    MC::init();

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
