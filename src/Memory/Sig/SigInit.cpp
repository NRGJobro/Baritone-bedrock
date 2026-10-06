#include "SigInit.h"

#include "SignatureManager.h"

void SigInit::addSigs() {
    initHooks();
    initRender();
    initRest();
}

void SigInit::initRender() {
    ADD_SIG("mce::Mesh::_renderMesh",
        "E8 ? ? ? ? F3 0F 5C F7 F3 0F 58 F7");
    ADD_SIG("mce::RenderMaterialGroup::common",
        "48 8D 15 ? ? ? ? 4C 8D 45 ? E8 ? ? ? ? 48 8D 4D ? E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? E9 ? ? ? ? 48 8D 0D");
}

void SigInit::initHooks() {
    ADD_SIG("RenderContextHook::ctxSig",
        "E8 ? ? ? ? 48 8B 4B ? 48 85 C9 74 ? 48 8B 01 48 8B 40 ? 48 89 FA FF 15 ? ? ? ? 48 8D 4D");
    ADD_SIG("WindowProcCallbackHook::keymapSig",
        "55 41 57 41 56 41 55 41 54 56 57 53 48 81 EC ? ? ? ? 48 8D AC 24 ? ? ? ? 48 C7 85 ? ? ? ? ? ? ? ? 89 D6 4C 8B 3D");
    ADD_SIG("GrabMouseHook::grabMouseSig",
        "55 41 56 56 57 53 48 83 EC ? 48 8D 6C 24 ? 48 C7 45 ? ? ? ? ? 80 B9 ? ? ? ? ? 0F 85 ? ? ? ? 48 89 CE 48 83 C1");
    ADD_SIG("UpdateHook::updateSig",
        "55 41 57 41 56 41 55 41 54 56 57 53 48 81 EC ? ? ? ? 48 8D AC 24 ? ? ? ? 48 C7 85 ? ? ? ? ? ? ? ? 89 D3 48 89 CE 48 8B 01");
    ADD_SIG("LevelRendererHook::levelRendererHookSig",
        "E8 ? ? ? ? 45 31 E4 48 83 BE");
    ADD_SIG("CameraOriginHook::tickSig",
        "41 57 41 56 41 54 56 57 55 53 48 81 EC ? ? ? ? 44 0F 29 BC 24 ? ? ? ? 44 0F 29 B4 24 ? ? ? ? 44 0F 29 AC 24 ? ? ? ? 44 0F 29 A4 24 ? ? ? ? 44 0F 29 9C 24 ? ? ? ? 44 0F 29 94 24 ? ? ? ? 44 0F 29 8C 24 ? ? ? ? 44 0F 29 44 24 ? 0F 29 7C 24 ? 0F 29 74 24 ? 0F 28 F2");
    ADD_SIG("PerspectiveHook::perspectiveSig",
        "48 83 EC 38 48 8B 05 ? ? ? ? 48 31 E0 48 89 44 24 ? 48 8B 01 48 8B 40 08 48 8D 54 24 ? 41 B8 03 00 00 00");
    ADD_SIG("GammaHook::gammaSig",
        "48 83 EC 38 48 8B 05 ? ? ? ? 48 31 E0 48 89 44 24 ? 48 8B 01 48 8B 40 08 48 8D 54 24 ? 41 B8 32 00 00 00");
    ADD_SIG("WorldNotShowingMenusHook::worldMenusSig",
        "48 8B 49 ? 48 8B 01 48 8B 80 ? ? ? ? 48 8B 15 ? ? ? ? 48 FF E2 CC CC CC CC CC CC CC CC 56 57 48 83 EC ? 48 89 CE");
    ADD_SIG("MouseHook::mouseSig",
        "55 41 57 41 56 41 55 41 54 56 57 53 48 81 EC ? ? ? ? 48 8D AC 24 ? ? ? ? 44 0F 29 BD ? ? ? ? 44 0F 29 B5 ? ? ? ? 44 0F 29 AD ? ? ? ? 44 0F 29 A5 ? ? ? ? 44 0F 29 9D ? ? ? ? 44 0F 29 95 ? ? ? ? 44 0F 29 8D ? ? ? ? 44 0F 29 85 ? ? ? ? 0F 29 BD ? ? ? ? 0F 29 B5 ? ? ? ? 48 C7 85 ? ? ? ? ? ? ? ? 48 89 CE 8B 05");
    ADD_SIG("MouseDevice::instance",
        "89 15 ? ? ? ? C7 47");
}

void SigInit::initRest() {
    ADD_SIG("Platform_GameCore::instanceReference",
        "4C 89 3D ? ? ? ? 4D 85 FF");
    ADD_SIG("GuiData::displayClientMessage",
        "55 56 53 48 81 EC ? ? ? ? 48 8D AC 24 ? ? ? ? 48 C7 85 ? ? ? ? ? ? ? ? 44 88 CB");
    ADD_SIG("MinecraftPackets::createPacket",
        "56 48 83 EC ? 48 89 CE 81 FA");
}
