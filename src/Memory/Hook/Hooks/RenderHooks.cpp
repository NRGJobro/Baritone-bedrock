#include "RenderHooks.h"

#include "../../../Client.h"
#include "../../../Client/GUI/ClickGui.h"
#include "../../../Client/Modules/CameraTweaksModule.h"
#include "../../../Client/Modules/GuiMoveModule.h"
#include "../../../Client/Modules/LimiterModule.h"
#include "../../../Client/Modules/ModuleManager.h"
#include "../../../SDK/MC.h"
#include "../../../SDK/Render/MinecraftUIRenderContext.h"
#include "../../../SDK/Screen/ScreenView.h"
#include "../../../Utils/DrawUtils.h"
#include "../../../Utils/LimiterTess.h"
#include "../../../Utils/TimeUtils.h"
#include "../../../Utils/Utils.h"
#include "../HookManager.h"

namespace {

std::int64_t Camera_getPerspective(std::int64_t options) {
    HookManager::CallbackGuard callbackGuard;
    static auto original = GET_HOOK(&Camera_getPerspective);
    const auto value = original == nullptr ? 0 : original(options);
    if (!callbackGuard.allowClientCode())
        return value;
    if (auto* cameraTweaks = g_modMgr.getModule<CameraTweaksModule>(); cameraTweaks != nullptr)
        cameraTweaks->setPerspective(static_cast<int>(value));
    return value;
}

void CameraBlend_tick(MinecraftCamera::CameraComponent* camera, void* context, float deltaTime) {
    HookManager::CallbackGuard callbackGuard;
    static auto original = GET_HOOK(&CameraBlend_tick);
    if (original != nullptr)
        original(camera, context, deltaTime);
    if (!callbackGuard.allowClientCode())
        return;
    if (auto* cameraTweaks = g_modMgr.getModule<CameraTweaksModule>();
        cameraTweaks != nullptr && cameraTweaks->isEnabled())
        cameraTweaks->apply(camera);
}

void ScreenView_setupAndRender(ScreenView* screenView, MinecraftUIRenderContext* renderContext) {
    HookManager::CallbackGuard callbackGuard;
    static auto original = GET_HOOK(&ScreenView_setupAndRender);
    if (original == nullptr)
        return;
    original(screenView, renderContext);

    if (!callbackGuard.allowClientCode())
        return;
    if (screenView == nullptr || renderContext == nullptr)
        return;
    const auto tree = screenView->getVisualTree();
    const auto root = tree == nullptr ? nullptr : tree->getRootControl();
    if (root == nullptr)
        return;

    static std::string currentScreen;
    const auto& name = root->getName();
    if (name != "debug_screen" && name != "toast_screen" && name != "modal_progress_screen")
        currentScreen = name;
    g_Client.hudScreenActive.store(currentScreen == "hud_screen",
        std::memory_order_release);
    const auto limiter = g_modMgr.getModule<LimiterModule>();
    const auto guiMove = g_modMgr.getModule<GuiMoveModule>();
    const bool inventoryMove = guiMove != nullptr && guiMove->isEnabled() &&
        MC::getLocalPlayer() != nullptr;
    g_Client.gameplayInputAllowed.store(
        !g_Client.clickGuiOpened && (currentScreen == "hud_screen" || inventoryMove),
        std::memory_order_release);
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
        if (limiter != nullptr && limiter->isEnabled() && !g_Client.clickGuiOpened) {
            const auto status = "Limiter: " + limiter->getController().getStatusLine();
            DrawUtils::drawText(status, {4.f, 4.f}, {0.85f, 0.95f, 1.f, 1.f}, 0.85f);
        }
        // drawText queues glyph meshes on the current UI context. The native
        // screen already flushed before this post-render hook, so flush our
        // batch explicitly just as Phase does for custom HUD text.
        renderContext->flushText();
    } else {
        if (g_Client.clickGuiOpened)
            ClickGui::setOpen(false);
    }
}

__int64 LevelRenderer_renderLevel(LevelRenderer* renderer, ScreenContext* screenContext, const __int64 frame) {
    HookManager::CallbackGuard callbackGuard;
    static auto original = GET_HOOK(&LevelRenderer_renderLevel);
    if (original == nullptr)
        return 0;

    if (!callbackGuard.allowClientCode())
        return original(renderer, screenContext, frame);

    bool renderOverrideStarted = false;
    try {
        g_modMgr.onBeforeRenderLevel();
        renderOverrideStarted = true;
    } catch (...) {
        // Never allow client-side UI/path rendering exceptions to cross the
        // Minecraft render callback boundary.
    }

    const auto result = original(renderer, screenContext, frame);

    if (renderOverrideStarted) {
        try {
            g_modMgr.onAfterRenderLevel();
        } catch (...) {
        }
    }

    if (screenContext != nullptr) {
        try {
            DrawUtils::update(screenContext);
            LimiterTess::setTessellator3D(screenContext);
            g_modMgr.onRenderLevel();
        } catch (...) {
            // Rendering is optional. A bad frame must not take Minecraft down.
        }
        LimiterTess::setTessellator3D(nullptr);
    }
    return result;
}

} // namespace

void RenderHooks::init() {
    ADD_HOOK("CameraOriginHook::tickSig", CameraBlend_tick);
    ADD_HOOK("PerspectiveHook::perspectiveSig", Camera_getPerspective);
    ADD_HOOK2(ScreenView_setupAndRender,
        Utils::getFromOffset<uintptr_t>(GET_SIG("RenderContextHook::ctxSig"), 1));
    ADD_HOOK2(LevelRenderer_renderLevel,
        Utils::getFromOffset<uintptr_t>(GET_SIG("LevelRendererHook::levelRendererHookSig"), 1));
}
