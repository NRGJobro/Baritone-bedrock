#include "SigInit.h"

#include "SignatureManager.h"

void SigInit::addSigs() {
    initHooks();
    initRender();
    initRest();
}

void SigInit::initRender() {
    ADD_SIG("mce::Mesh::_renderMesh", "E8 ? ? ? ? E9 ? ? ? ? 44 0F B6 83");

    ADD_SIG("mce::RenderMaterialGroup::common", "48 8D 15 ? ? ? ? E8 ? ? ? ? 90 49 8D 8E ? ? ? ? E8");

    ADD_SIG("cg::ImageResourceVtable", "48 8D 05 ? ? ? ? 48 89 43 ? 48 89 29");

    ADD_SIG("mce::TextureGroup::uploadTexture", "40 53 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 49 8B D9 49 8B F0");

    ADD_SIG("mce::LRUCache::remove", "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 55 41 56 41 57 48 83 EC ? 48 8B EA 48 8B F1 45 33 ED");

    ADD_SIG("bgfx::d3d11::RendererContextD3D11::updateResolution::swapChainFlags", "C7 44 24 ? ? ? ? ? 89 4C 24 ? 49 8B CA");

    ADD_SIG("Font::drawShadow", "E8 ? ? ? ? E9 ? ? ? ? 44 88 6D");

    ADD_SIG("BaseActorRenderContext::BaseActorRenderContext", "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 4C 24 ? 57 48 83 EC ? 49 8B F8 48 8B DA 48 8B F1 48 8D 05 ? ? ? ? 48 89 01 33 ED");

    ADD_SIG("ItemRenderer::renderGuiItemNew", "E8 ? ? ? ? 4C 8D 4C 24 ? 4C 8D 44 24 ? 48 8B D5 E8 ? ? ? ? 80 BF");
    ADD_SIG("ItemRenderer::iconBlit", "E8 ? ? ? ? EB ? 49 8B 8C 24");
    ADD_SIG("ItemRenderer::getGraphics", "48 89 5C 24 ? 57 48 83 EC ? 48 8B DA 48 8B F9 E8 ? ? ? ? 80 7B ? ? 75");

    ADD_SIG("ItemIconManager::getIcon", "40 53 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 45 0F B6 C8");
}

void SigInit::initHooks() {
	ADD_SIG("ScreenView::setupAndRender", "E8 ? ? ? ? 48 8B 4B ? 48 85 C9 74 ? 48 8B 01 48 8B D6");
    ADD_SIG("MainWindow::_windowProcCallback", "40 53 56 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 48 8B F9");
    ADD_SIG("GameCore::handleMouseInput", "48 8B C4 48 89 58 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 0F 29 70 ? 0F 29 78 ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 4C 89 45");
    ADD_SIG("bgfx::dxgi::createSwapChain", "40 55 53 56 57 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 4C 8B F1 C6 44 24");
    // UNUSED ADD_SIG("MinecraftUIRenderContext::drawImage", "E8 ? ? ? ? 49 8B CF E8 ? ? ? ? F3 41 0F 10 47");
    // UNUSED ADD_SIG("MinecraftUIRenderContext::drawNineslice", "48 89 5C 24 ? 48 89 7C 24 ? 55 48 8B EC 48 83 EC ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 49 8D B8");
    ADD_SIG("bgfx::d3d11::RendererContextD3D11::flip", "40 57 48 83 EC ? 48 83 B9 ? ? ? ? ? 48 8B F9 0F 84");
    ADD_SIG("bgfx::d3d12::RendererContextD3D12::flip", "48 8B C4 57 48 83 EC ? 48 83 B9");
    ADD_SIG("MinecraftGame::onDeviceLost", "48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC ? 4C 8B F9 33 F6");
	// UNUSED ADD_SIG("BaseLightTextureImageBuilder::createBaseLightTextureData", "48 89 5C 24 ? 48 89 54 24 ? 55 56 57 41 56 41 57 48 83 EC ? 4D 8B F1 49 8B F8");
	// UNUSED ADD_SIG("NetherLightTextureImageBuilder::createBaseLightTextureData", "48 89 5C 24 ? 48 89 54 24 ? 55 56 57 41 56 41 57 48 83 EC ? 49 8B E9");
	ADD_SIG("mce::framebuilder::bgfxbridge::BgfxFrameExtractor::insert", "48 89 5C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 48 0F BE 42 ? 4C 8B C9");
    ADD_SIG("LevelRenderer::renderLevel", "E8 ? ? ? ? 45 32 F6 48 8B 8E");
    ADD_SIG("ClientInstanceScreenModel::sendChatMessage", "48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 4C 8B FA 4C 8B F1 45 33 E4 48 8B 49");
    ADD_SIG("MultiPlayerLevel::_subTick", "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 0F 29 74 24 ? 0F 29 7C 24 ? 44 0F 29 44 24 ? 4C 8B F1");
    ADD_SIG("DataDrivenRendererV2::generateFrame", "E8 ? ? ? ? 8B D8 E8 ? ? ? ? 8B 4C 24");
}

void SigInit::initRest() {
    ADD_SIG("mMainWindow", "48 89 0D ? ? ? ? 48 85 C9");

    ADD_SIG("GuiData::displayClientMessage", "40 55 53 56 57 41 56 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 41 0F B6 ? 49 8B D8");

    ADD_SIG("mce::Image::clone", "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 83 EC ? 4C 8B F2 48 8B D9");

    ADD_SIG("MouseDevice::_instance", "89 0D ? ? ? ? 41 B7");

    ADD_SIG("MapItem::sampleMapData", "E8 ? ? ? ? 48 8B AC 24 ? ? ? ? 40 84 F6");

    ADD_SIG("Block::getBlockMapColorComponent", "E8 ? ? ? ? 48 8B F0 48 85 C0 74 ? 0F B6 48");

    ADD_SIG("BlockSource::getBiome", "E8 ? ? ? ? 80 78 ? ? 0F 84 ? ? ? ? 48 8B 46");

    ADD_SIG("Biome::getMapFoliageColor", "40 53 48 83 EC ? 65 48 8B 04 25 ? ? ? ? 48 8B D9 B9 ? ? ? ? 48 8B 00 8B 04 01 39 05 ? ? ? ? 0F 8F ? ? ? ? 48 8B 83 ? ? ? ? 48 8B 8B ? ? ? ? 0F B7 15 ? ? ? ? 48 3B C1 74 ? 66 39 10 74 ? 48 83 C0 ? 48 3B C1 75 ? F3 0F 10 53");
    ADD_SIG("Biome::getMapGrassColor", "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 83 EC ? 65 48 8B 04 25 ? ? ? ? 48 8B EA");

    ADD_SIG("LevelChunk::getTopRainBlockPos", "40 55 56 41 54 41 56 41 57 48 83 EC ? 41 0F B6 70");

    ADD_SIG("MinecraftPackets::createPacket", "48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8B EC 48 83 EC ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 48 8B F9");

    ADD_SIG("BlockSource::fetchBlocks", "4C 8B DC 53 41 54 41 55 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 44 8B 0A");

    ADD_SIG("Actor::getAttachPos", "40 53 48 83 EC 20 0F 57 DB 48 8B DA");

    ADD_SIG("ItemStackBase::getColor", "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 4C 8B FA 48 8B F1 33 FF");
}
