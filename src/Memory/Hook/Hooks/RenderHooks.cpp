#include "RenderHooks.h"

#include "../../../Client.h"
#include "../../../Client/GUI/ClickGui.h"
#include "../../../Client/Module/ModuleManager.h"
#include "../../../Client/Module/Modules/BaritoneModule.h"
#include "../../../SDK/MC.h"
#include "../../../SDK/Render/MinecraftUIRenderContext.h"
#include "../../../SDK/Screen/ScreenView.h"
#include "../../../Utils/DrawUtils.h"
#include "../../../Utils/LimiterTess.h"
#include "../../../Utils/TimeUtils.h"
#include "../../../Utils/Utils.h"
#include "../HookManager.h"

namespace {

void ScreenView_setupAndRender(ScreenView* screenView, MinecraftUIRenderContext* renderContext) {
    static auto original = GET_HOOK(&ScreenView_setupAndRender);
    original(screenView, renderContext);

    static std::string currentScreen;
    const auto& name = screenView->getVisualTree()->getRootControl()->getName();
    if (name != "debug_screen" && name != "toast_screen" && name != "modal_progress_screen")
        currentScreen = name;

    if (name != "debug_screen")
        return;

    DrawUtils::updateMCUIRC(renderContext);
    DrawUtils::setShaderColor();

    static auto lastFrame = TimeUtils::currentTimeMillis();
    const auto now = TimeUtils::currentTimeMillis();
    g_Client.deltaTime = std::clamp(static_cast<double>(now - lastFrame) / 1000.0, 0.0, 0.1);
    lastFrame = now;

    if (currentScreen == "hud_screen") {
        ClickGui::render();

        const auto module = g_modMgr.getModule<BaritoneModule>();
        if (module != nullptr && module->isEnabled() && !g_Client.clickGuiOpened) {
            const auto status = "Limiter: " + module->getController().getStatusLine();
            DrawUtils::drawText(status, {4.f, 4.f}, {0.85f, 0.95f, 1.f, 1.f}, 0.85f);
        }
    } else {
        g_Client.clickGuiOpened = false;
    }
}

void LevelRenderer_renderLevel(LevelRenderer* renderer, ScreenContext* screenContext, void* frame) {
    static auto original = GET_HOOK(&LevelRenderer_renderLevel);
    g_modMgr.onBeforeRenderLevel();
    original(renderer, screenContext, frame);
    g_modMgr.onAfterRenderLevel();
    DrawUtils::update(screenContext);
    LimiterTess::setTessellator3D(screenContext);
    g_modMgr.onRenderLevel();
    LimiterTess::setTessellator3D(nullptr);
}

} // namespace

void RenderHooks::init() {
    ADD_HOOK2(ScreenView_setupAndRender, Utils::getFromOffset<uintptr_t>(GET_SIG("ScreenView::setupAndRender"), 1));
    ADD_HOOK2(LevelRenderer_renderLevel, Utils::getFromOffset<uintptr_t>(GET_SIG("LevelRenderer::renderLevel"), 1));
}
