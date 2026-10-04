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
    ADD_SIG("mce::TextureGroup::uploadTexture",
        "55 41 57 41 56 41 55 41 54 56 57 53 48 81 EC ? ? ? ? 48 8D AC 24 ? ? ? ? 48 C7 85 ? ? ? ? ? ? ? ? 4D 89 CF 4D 89 C6 48 89 D6 48 89 CB");
    ADD_SIG("mce::LRUCache::remove",
        "41 57 41 56 56 57 53 48 83 EC ? 48 89 CE 48 8D 79 ? 4C 8B 42");
}

void SigInit::initHooks() {
    ADD_SIG("RenderContextHook::ctxSig",
        "E8 ? ? ? ? 48 8B 4B ? 48 85 C9 74 ? 48 8B 01 48 8B 40 ? 48 89 FA FF 15 ? ? ? ? 48 8D 4D");
    ADD_SIG("WindowProcCallbackHook::keymapSig",
        "55 41 57 41 56 41 55 41 54 56 57 53 48 81 EC ? ? ? ? 48 8D AC 24 ? ? ? ? 48 C7 85 ? ? ? ? ? ? ? ? 89 D6 4C 8B 3D");
    ADD_SIG("UpdateHook::updateSig",
        "55 41 57 41 56 41 55 41 54 56 57 53 48 81 EC ? ? ? ? 48 8D AC 24 ? ? ? ? 48 C7 85 ? ? ? ? ? ? ? ? 89 D3 48 89 CE 48 8B 01");
    ADD_SIG("LevelRendererHook::levelRendererHookSig",
        "E8 ? ? ? ? 45 31 E4 48 83 BE");
    ADD_SIG("GammaHook::gammaSig",
        "48 83 EC 38 48 8B 05 ? ? ? ? 48 31 E0 48 89 44 24 ? 48 8B 01 48 8B 40 08 48 8D 54 24 ? 41 B8 32 00 00 00");
}

void SigInit::initRest() {
    ADD_SIG("Platform_GameCore::instanceReference",
        "4C 89 3D ? ? ? ? 4D 85 FF");
    ADD_SIG("GuiData::displayClientMessage",
        "55 56 53 48 81 EC ? ? ? ? 48 8D AC 24 ? ? ? ? 48 C7 85 ? ? ? ? ? ? ? ? 44 88 CB");
    ADD_SIG("MinecraftPackets::createPacket",
        "56 48 83 EC ? 48 89 CE 81 FA");
}
