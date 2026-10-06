#include "ClickGui.h"

#include "../../Client.h"
#include "../../SDK/MC.h"
#include "../../SDK/Render/MeshHelpers.h"
#include "../../Utils/DrawUtils.h"
#include "../Modules/LimiterModule.h"
#include "../Modules/ModuleManager.h"

namespace {

    constexpr float meshSmoothness = 30.f;
    constexpr float shellRadius = 6.f;
    constexpr float cardRadius = 4.f;
    constexpr int modulesPerRow = 1;
    constexpr float cardGap = 12.f;

    const mce::Color ink{0.98f, 0.98f, 1.f, 1.f};
    const mce::Color muted{0.73f, 0.70f, 0.80f, 1.f};
    const mce::Color dim{0.50f, 0.46f, 0.58f, 1.f};
    const mce::Color redline{0.60f, 0.25f, 1.f, 1.f};
    const mce::Color amber{0.82f, 0.58f, 1.f, 1.f};
    const mce::Color green{0.43f, 1.f, 0.69f, 1.f};

    float guiX = 0.f;
    float guiY = 0.f;
    float guiWidth = 0.f;
    float guiHeight = 0.f;
    float sidebarWidth = 0.f;
    float headerHeight = 0.f;
    float scrollOffset = 0.f;
    float renderedScrollOffset = 0.f;
    float settingsScrollOffset = 0.f;
    float renderedSettingsScrollOffset = 0.f;
    float maxSettingsScroll = 0.f;
    int wheelDirection = 0;

    glm::vec2 contentPos{};
    glm::vec2 contentSize{};
    glm::vec2 moduleSize{};
    glm::vec2 settingsPos{};
    glm::vec2 settingsSize{};
    glm::vec2 lastUiSize{};
    glm::vec2 mousePos{};

    bool clickPending = false;
    bool rightClickPending = false;
    bool settingsOpen = false;
    bool rotationDragging = false;
    bool bridgeDragging = false;
    bool closingAnimation = false;
    float openAnimation = 0.f;
    std::unordered_map<const Module*, float> hoverAnimations;
    std::unordered_map<const bool*, float> toggleAnimations;

    bool contains(const glm::vec4& box, const glm::vec2 point) {
        return point.x >= box.x && point.x < box.x + box.z && point.y >= box.y && point.y < box.y + box.w;
    }

    void renderAt(mce::Mesh& mesh, const glm::vec2 position, ScreenContext* screenContext, mce::MaterialPtr* material, MatrixStack& stack) {
        stack.push();
        stack.top().matrix = translate(stack.top().matrix, {position.x, position.y, 0.f});
        mesh.renderMesh(screenContext->toMeshContext(), material);
        stack.pop();
    }

    std::string fitText(std::string text, const float maximumWidth, const float scale) {
        if (DrawUtils::getTextWidth(text, scale) <= maximumWidth)
            return text;
        constexpr std::string_view ellipsis = "...";
        while (!text.empty() && DrawUtils::getTextWidth(text + std::string(ellipsis), scale) > maximumWidth)
            text.pop_back();
        return text + std::string(ellipsis);
    }

    void drawImmediateBars(const std::initializer_list<std::pair<glm::vec4, mce::Color>>& bars) {
        auto* tessellator = DrawUtils::getTessellator();
        if (tessellator == nullptr)
            return;
        tessellator->begin();
        for (const auto& [rectangle, color] : bars)
            DrawUtils::addFilledRectangle(rectangle, color, color.a);
        MeshHelpers::renderMeshImmediately(DrawUtils::getScreenContext(), tessellator, DrawUtils::getUIFillColor());
    }

}  // namespace

void ClickGui::setOpen(const bool open) {
    if (g_Client.clickGuiOpened == open) {
        if (open)
            maintainMouseCapture();
        return;
    }

    g_Client.clickGuiOpened = open;
    const auto window = MC::getWindowHandle();
    if (open) {
        closingAnimation = false;
        g_Client.gameplayInputAllowed.store(false, std::memory_order_release);
        if (auto* client = MC::getClientInstance(); client != nullptr)
            client->releaseMouse();
        ClipCursor(nullptr);
        if (IsWindow(window))
            SetCapture(window);
        SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)));
        clickPending = false;
        rightClickPending = false;
    } else {
        closingAnimation = true;
        settingsOpen = false;
        rotationDragging = false;
        bridgeDragging = false;
        if (GetCapture() == window)
            ReleaseCapture();
        if (auto* client = MC::getClientInstance(); client != nullptr)
            client->grabMouse();
    }
}

void ClickGui::maintainMouseCapture() {
    const auto window = MC::getWindowHandle();
    ClipCursor(nullptr);
    if (IsWindow(window) && GetCapture() != window)
        SetCapture(window);
    SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)));
}

void ClickGui::render() {
    if (g_Client.clickGuiOpened)
        maintainMouseCapture();

    auto* screenContext = DrawUtils::getScreenContext();
    auto* tessellator = DrawUtils::getTessellator();
    auto* material = DrawUtils::getUIFillColor();
    auto* guiData = MC::getGuiData();
    auto* client = MC::getClientInstance();
    if ((!g_Client.clickGuiOpened && !closingAnimation) || screenContext == nullptr || tessellator == nullptr || material == nullptr || guiData == nullptr || client == nullptr) {
        renderedScrollOffset = scrollOffset;
        renderedSettingsScrollOffset = settingsScrollOffset;
        return;
    }

    const auto clientScreenSize = guiData->screenSizeData.clientScreenSize;
    const auto uiSize = guiData->screenSizeData.clientUIScreenSize;
    if (clientScreenSize.x <= 0.f || clientScreenSize.y <= 0.f || uiSize.x <= 0.f || uiSize.y <= 0.f)
        return;

    const float animationStep = std::clamp(static_cast<float>(g_Client.deltaTime) * 12.f, 0.f, 1.f);
    openAnimation += ((g_Client.clickGuiOpened ? 1.f : 0.f) - openAnimation) * animationStep;
    if (closingAnimation && openAnimation <= 0.015f) {
        openAnimation = 0.f;
        closingAnimation = false;
        return;
    }
    const float easedOpen = 1.f - std::pow(1.f - openAnimation, 3.f);

    POINT cursor{};
    if (GetCursorPos(&cursor) && ScreenToClient(MC::getWindowHandle(), &cursor))
        mousePos = {static_cast<float>(cursor.x), static_cast<float>(cursor.y)};
    mousePos = mousePos / clientScreenSize * uiSize;

    guiWidth = std::clamp(uiSize.x * 0.70f, 610.f, 790.f);
    guiHeight = std::clamp(uiSize.y * 0.72f, 390.f, 500.f);
    guiX = (uiSize.x - guiWidth) * 0.5f;
    guiY = (uiSize.y - guiHeight) * 0.5f + (1.f - easedOpen) * 18.f;
    sidebarWidth = std::clamp(guiWidth * 0.235f, 158.f, 184.f);
    headerHeight = 66.f;
    contentPos = {guiX + sidebarWidth, guiY + headerHeight};
    contentSize = {guiWidth - sidebarWidth, guiHeight - headerHeight};
    moduleSize = {contentSize.x - cardGap * 2.f, 124.f};
    settingsPos = {contentPos.x + cardGap, contentPos.y + cardGap};
    settingsSize = {contentSize.x - cardGap * 2.f, contentSize.y - cardGap * 2.f};

    if (!builtMeshes || lastUiSize != uiSize || openAnimation < 0.995f || closingAnimation) {
        lastUiSize = uiSize;
        buildMeshes();
        builtMeshes = true;
    }

    auto* meshContext = screenContext->toMeshContext();
    if (meshContext == nullptr)
        return;

    overlayMesh.renderMesh(meshContext, material);
    shellMesh.renderMesh(meshContext, material);
    headerMesh.renderMesh(meshContext, material);
    sidebarMesh.renderMesh(meshContext, material);
    redlineMesh.renderMesh(meshContext, material);

    auto& stack = client->getCamera().worldMatrixStack;

    auto* limiter = g_modMgr.getModule<LimiterModule>();
    int activeModules = 0;
    for (const auto& module : g_modMgr.getSortedModules())
        activeModules += module->isEnabled() ? 1 : 0;

    DrawUtils::drawText("LIMITER", {guiX + 20.f, guiY + 18.f}, ink, 1.30f);
    DrawUtils::drawText("DRIVER CONTROL", {guiX + 106.f, guiY + 24.f}, muted, 0.76f);
    DrawUtils::drawText("PATH CORE", {guiX + guiWidth - 86.f, guiY + 22.f}, amber, 0.76f);

    renderAt(gaugeMesh, {guiX + sidebarWidth * 0.5f, guiY + 126.f}, screenContext, material, stack);
    const std::string activeLabel = std::format("{} / {}", activeModules, g_modMgr.getModuleCount());
    DrawUtils::drawText(activeLabel, {guiX + sidebarWidth * 0.5f - DrawUtils::getTextWidth(activeLabel, 1.26f) * 0.5f, guiY + 116.f}, activeModules > 0 ? green : ink, 1.26f);
    DrawUtils::drawText("SYSTEMS ARMED", {guiX + sidebarWidth * 0.5f - DrawUtils::getTextWidth("SYSTEMS ARMED", 0.72f) * 0.5f, guiY + 144.f}, muted, 0.72f);

    const glm::vec4 garageTab{guiX + 13.f, guiY + 177.f, sidebarWidth - 26.f, 34.f};
    drawImmediateBars({{{garageTab.x, garageTab.y, garageTab.x + garageTab.z, garageTab.y + garageTab.w}, {0.12f, 0.13f, 0.14f, 0.96f}}, {{garageTab.x, garageTab.y, garageTab.x + 3.f, garageTab.y + garageTab.w}, redline}});
    DrawUtils::drawText("01", {garageTab.x + 12.f, garageTab.y + 9.f}, amber, 0.72f);
    DrawUtils::drawText("GARAGE", {garageTab.x + 40.f, garageTab.y + 8.f}, ink, 0.88f);
    DrawUtils::drawText("PATHFINDING CORE", {guiX + 20.f, guiY + 228.f}, muted, 0.70f);
    DrawUtils::drawText("TAB", {guiX + 20.f, guiY + guiHeight - 43.f}, amber, 0.72f);
    DrawUtils::drawText("CLOSE", {guiX + 53.f, guiY + guiHeight - 43.f}, ink, 0.72f);
    DrawUtils::drawText("BEDROCK EDITION", {guiX + 20.f, guiY + guiHeight - 23.f}, muted, 0.70f);

    const float titleX = contentPos.x + 18.f;
    DrawUtils::drawText(settingsOpen ? "TUNING BAY" : "SYSTEM GARAGE", {titleX, guiY + 20.f}, ink, 1.02f);
    if (limiter != nullptr) {
        const auto status = fitText(limiter->getController().getStatusLine(), std::min(200.f, contentSize.x - 230.f), 0.72f);
        DrawUtils::drawText(status, {contentPos.x + contentSize.x - DrawUtils::getTextWidth(status, 0.72f) - 18.f, guiY + 24.f}, limiter->isEnabled() ? green : muted, 0.72f);
    }

    if (!settingsOpen) {
        constexpr float wheelSpeed = 22.f;
        scrollOffset += wheelDirection * wheelSpeed;
        wheelDirection = 0;
        const int rows = (static_cast<int>(g_modMgr.getModuleCount()) + modulesPerRow - 1) / modulesPerRow;
        const float gridHeight = cardGap + rows * (moduleSize.y + cardGap);
        const float maxModuleScroll = std::max(0.f, gridHeight - contentSize.y);
        scrollOffset = std::clamp(scrollOffset, 0.f, maxModuleScroll);
        renderedScrollOffset += (scrollOffset - renderedScrollOffset) * std::clamp(static_cast<float>(g_Client.deltaTime) * 15.f, 0.f, 1.f);

        screenContext->setClippingRectangle(contentPos.x, contentPos.y, contentPos.x + contentSize.x, contentPos.y + contentSize.y);
        int cardIndex = 0;
        for (const auto& module : g_modMgr.getSortedModules()) {
            const int row = cardIndex / modulesPerRow;
            const int column = cardIndex % modulesPerRow;
            ++cardIndex;
            const glm::vec2 cardPosition{contentPos.x + cardGap + column * (moduleSize.x + cardGap), contentPos.y + cardGap + row * (moduleSize.y + cardGap) - renderedScrollOffset};
            const glm::vec4 cardBox{cardPosition.x, cardPosition.y, moduleSize.x, moduleSize.y};
            if (cardPosition.y < contentPos.y || cardPosition.y + moduleSize.y > contentPos.y + contentSize.y)
                continue;

            const bool hovered = contains(cardBox, mousePos);
            auto& hoverAmount = hoverAnimations[module.get()];
            hoverAmount += ((hovered ? 1.f : 0.f) - hoverAmount) * std::clamp(static_cast<float>(g_Client.deltaTime) * 16.f, 0.f, 1.f);
            module->enabledButtonRegion += ((module->isEnabled() ? 1.f : 0.f) - module->enabledButtonRegion) * std::clamp(static_cast<float>(g_Client.deltaTime) * 12.f, 0.f, 1.f);

            renderAt(cardMesh, cardPosition, screenContext, material, stack);
            if (hoverAmount > 0.01f) {
                DrawUtils::setShaderColor(1.f, 1.f, 1.f, hoverAmount);
                renderAt(cardHoverMesh, cardPosition, screenContext, material, stack);
                DrawUtils::setShaderColor();
            }
            if (module->enabledButtonRegion > 0.01f) {
                DrawUtils::setShaderColor(1.f, 1.f, 1.f, module->enabledButtonRegion);
                renderAt(cardActiveMesh, cardPosition, screenContext, material, stack);
                DrawUtils::setShaderColor();
            }

            const std::string number = std::format("{:02}", cardIndex);
            DrawUtils::drawText(number, {cardPosition.x + 14.f, cardPosition.y + 13.f}, module->isEnabled() ? amber : muted, 0.72f);
            DrawUtils::drawText(module->getName(), {cardPosition.x + 46.f, cardPosition.y + 10.f}, ink, 1.10f);
            DrawUtils::drawText(fitText(module->getDescription(), moduleSize.x - 30.f, 0.76f), {cardPosition.x + 15.f, cardPosition.y + 42.f}, muted, 0.76f);

            const glm::vec4 powerBox{cardPosition.x + 15.f, cardPosition.y + moduleSize.y - 31.f, 72.f, 20.f};
            drawImmediateBars({{{powerBox.x, powerBox.y, powerBox.x + powerBox.z, powerBox.y + powerBox.w}, module->isEnabled() ? mce::Color{0.22f, 0.10f, 0.34f, 1.f} : mce::Color{0.10f, 0.07f, 0.13f, 1.f}}, {{powerBox.x, powerBox.y + powerBox.w - 2.f, powerBox.x + powerBox.z, powerBox.y + powerBox.w}, module->isEnabled() ? redline : dim}});
            DrawUtils::drawText(module->isEnabled() ? "ENABLED" : "DISABLED", {powerBox.x + 10.f, powerBox.y + 5.f}, module->isEnabled() ? amber : ink, 0.72f);

            if (limiter != nullptr && module.get() == limiter) {
                const std::string tune = "RIGHT CLICK: TUNE  >";
                DrawUtils::drawText(tune, {cardPosition.x + moduleSize.x - DrawUtils::getTextWidth(tune, 0.74f) - 15.f, powerBox.y + 5.f}, hovered ? ink : amber, 0.74f);
            }

            if (clickPending && hovered) {
                module->toggle();
                clickPending = false;
            }
            if (rightClickPending && hovered && limiter != nullptr && module.get() == limiter) {
                settingsOpen = true;
                settingsScrollOffset = 0.f;
                renderedSettingsScrollOffset = 0.f;
                rightClickPending = false;
            }
        }
        screenContext->resetClippingRectangle();

        if (maxModuleScroll > 0.5f) {
            const float trackTop = contentPos.y + 12.f;
            const float trackHeight = contentSize.y - 24.f;
            const float thumbHeight = std::max(28.f, trackHeight * (contentSize.y / gridHeight));
            const float thumbTravel = trackHeight - thumbHeight;
            const float scrollProgress = std::clamp(renderedScrollOffset / maxModuleScroll, 0.f, 1.f);
            const float thumbTop = trackTop + thumbTravel * scrollProgress;
            const float trackX = contentPos.x + contentSize.x - 5.f;

            drawImmediateBars({{{trackX, trackTop, trackX + 2.f, trackTop + trackHeight}, {0.20f, 0.13f, 0.27f, 0.78f}}, {{trackX, thumbTop, trackX + 2.f, thumbTop + thumbHeight}, redline}});

            if (renderedScrollOffset < maxModuleScroll - 1.f) {
                const std::string scrollHint = "SCROLL FOR MORE  v";
                const float hintWidth = DrawUtils::getTextWidth(scrollHint, 0.72f);
                const float hintX = contentPos.x + (contentSize.x - hintWidth) * 0.5f;
                const float hintY = contentPos.y + contentSize.y - 19.f;
                drawImmediateBars({{{hintX - 10.f, hintY - 4.f, hintX + hintWidth + 10.f, hintY + 15.f}, {0.06f, 0.025f, 0.09f, 0.94f}}, {{hintX - 10.f, hintY - 4.f, hintX + hintWidth + 10.f, hintY - 2.f}, redline}});
                DrawUtils::drawText(scrollHint, {hintX, hintY}, amber, 0.72f);
            }
        }
    } else if (limiter != nullptr) {
        screenContext->setClippingRectangle(settingsPos.x, settingsPos.y, settingsPos.x + settingsSize.x, settingsPos.y + settingsSize.y);
        renderAt(settingsMesh, settingsPos, screenContext, material, stack);

        settingsScrollOffset += wheelDirection * 20.f;
        wheelDirection = 0;
        maxSettingsScroll = std::max(0.f, 420.f - settingsSize.y);
        settingsScrollOffset = std::clamp(settingsScrollOffset, 0.f, maxSettingsScroll);
        renderedSettingsScrollOffset += (settingsScrollOffset - renderedSettingsScrollOffset) * std::clamp(static_cast<float>(g_Client.deltaTime) * 15.f, 0.f, 1.f);
        const float sy = settingsPos.y - renderedSettingsScrollOffset;
        const float clipTop = settingsPos.y + 5.f;
        const float clipBottom = settingsPos.y + settingsSize.y - 5.f;
        const auto fullyVisible = [&](const float y, const float height) { return y >= clipTop && y + height <= clipBottom; };

        auto& controller = limiter->getController();
        auto& path = controller.getOptions();
        auto& execution = controller.getExecutionOptions();
        auto& visuals = controller.getRenderOptions();
        struct Toggle {
            const char* label;
            bool* value;
        };
        const std::array dynamics{Toggle{"DIAGONAL LINES", &path.allowDiagonal}, Toggle{"WATER ROUTES", &path.allowWater}, Toggle{"STEP ASSIST", &path.allowAscend}, Toggle{"CONTROLLED DROPS", &path.allowFall}, Toggle{"PARKOUR", &path.allowParkour}, Toggle{"SPRINT", &execution.sprint}, Toggle{"AUTO REPLAN", &controller.getReplanWhenStuck()}, Toggle{"BRIDGE KIT", &path.allowBridge}};
        const std::array telemetry{Toggle{"ROUTE LINE", &visuals.renderPath}, Toggle{"GOAL MARKER", &visuals.renderGoal}, Toggle{"GOAL PULSE", &visuals.animatedGoal}, Toggle{"SEARCH TRACE", &visuals.renderCalculations}, Toggle{"X-RAY ROUTE", &visuals.renderThroughWalls}};

        if (fullyVisible(sy + 14.f, 12.f))
            DrawUtils::drawText("LIMITER / PERFORMANCE TUNING", {settingsPos.x + 18.f, sy + 14.f}, ink, 1.00f);
        if (fullyVisible(sy + 39.f, 10.f))
            DrawUtils::drawText("Track behavior, route telemetry, and response", {settingsPos.x + 18.f, sy + 39.f}, muted, 0.72f);

        const float columnGap = 24.f;
        const float columnWidth = (settingsSize.x - 60.f - columnGap) * 0.5f;
        const float leftX = settingsPos.x + 22.f;
        const float rightX = leftX + columnWidth + columnGap;
        const float rowsTop = sy + 84.f;
        constexpr float rowHeight = 27.f;

        auto drawToggleColumn = [&](const char* title, const auto& toggles, const float x) {
            if (fullyVisible(sy + 63.f, 10.f))
                DrawUtils::drawText(title, {x, sy + 63.f}, amber, 0.74f);
            for (std::size_t i = 0; i < toggles.size(); ++i) {
                const auto& toggle = toggles[i];
                auto& toggleAmount = toggleAnimations[toggle.value];
                toggleAmount += ((*toggle.value ? 1.f : 0.f) - toggleAmount) * std::clamp(static_cast<float>(g_Client.deltaTime) * 18.f, 0.f, 1.f);
                const float rowY = rowsTop + static_cast<float>(i) * rowHeight;
                if (!fullyVisible(rowY - 2.f, 16.f))
                    continue;
                const glm::vec4 rowBox{x - 4.f, rowY - 4.f, columnWidth + 8.f, 20.f};
                if (clickPending && contains(rowBox, mousePos)) {
                    *toggle.value = !*toggle.value;
                    clickPending = false;
                }
                DrawUtils::drawText(toggle.label, {x, rowY}, *toggle.value ? ink : muted, 0.74f);
                const float switchX = x + columnWidth - 30.f;
                DrawUtils::setShaderColor(0.30f + 0.30f * toggleAmount, 0.24f + 0.01f * toggleAmount, 0.36f + 0.64f * toggleAmount, 1.f);
                renderAt(toggleTrackMesh, {switchX, rowY - 1.f}, screenContext, material, stack);
                DrawUtils::setShaderColor();
                DrawUtils::setShaderColor(*toggle.value ? 1.f : 0.62f, *toggle.value ? 0.72f : 0.65f, *toggle.value ? 0.28f : 0.67f, 1.f);
                renderAt(knobMesh, {switchX + 7.f + 12.f * toggleAmount, rowY + 4.f}, screenContext, material, stack);
                DrawUtils::setShaderColor();
            }
        };
        drawToggleColumn("DRIVING DYNAMICS", dynamics, leftX);
        drawToggleColumn("TELEMETRY", telemetry, rightX);

        constexpr float minimumSmoothness = 0.25f;
        constexpr float maximumSmoothness = 10.f;
        constexpr int minimumBridge = 1;
        constexpr int maximumBridge = 16;
        const float sliderY = sy + 323.f;
        const float sliderWidth = columnWidth;
        const float bridgeFraction = static_cast<float>(path.maxBridgeLength - minimumBridge) / static_cast<float>(maximumBridge - minimumBridge);
        const float rotationFraction = std::clamp((execution.rotationSmoothness - minimumSmoothness) / (maximumSmoothness - minimumSmoothness), 0.f, 1.f);
        const glm::vec4 bridgeHit{leftX - 4.f, sliderY - 4.f, sliderWidth + 8.f, 34.f};
        const glm::vec4 rotationHit{rightX - 4.f, sliderY - 4.f, sliderWidth + 8.f, 34.f};
        const bool slidersVisible = fullyVisible(sliderY - 23.f, 39.f);
        if (slidersVisible && clickPending && contains(bridgeHit, mousePos)) {
            bridgeDragging = true;
            clickPending = false;
        } else if (slidersVisible && clickPending && contains(rotationHit, mousePos)) {
            rotationDragging = true;
            clickPending = false;
        }
        if (bridgeDragging) {
            const float fraction = std::clamp((mousePos.x - leftX) / sliderWidth, 0.f, 1.f);
            path.maxBridgeLength = std::clamp(static_cast<int>(std::round(minimumBridge + fraction * (maximumBridge - minimumBridge))), minimumBridge, maximumBridge);
        }
        if (rotationDragging) {
            const float fraction = std::clamp((mousePos.x - rightX) / sliderWidth, 0.f, 1.f);
            execution.rotationSmoothness = minimumSmoothness + fraction * (maximumSmoothness - minimumSmoothness);
        }

        const float trackY = sliderY + 10.f;
        if (slidersVisible) {
            DrawUtils::drawText(std::format("BRIDGE RANGE  {:02}", path.maxBridgeLength), {leftX, sliderY - 21.f}, ink, 0.72f);
            DrawUtils::drawText(std::format("STEERING RESPONSE  {:.2f}x", execution.rotationSmoothness), {rightX, sliderY - 21.f}, ink, 0.72f);
            drawImmediateBars({{{leftX, trackY - 2.f, leftX + sliderWidth, trackY + 2.f}, {0.19f, 0.21f, 0.22f, 1.f}}, {{leftX, trackY - 2.f, leftX + sliderWidth * bridgeFraction, trackY + 2.f}, redline}, {{rightX, trackY - 2.f, rightX + sliderWidth, trackY + 2.f}, {0.19f, 0.21f, 0.22f, 1.f}}, {{rightX, trackY - 2.f, rightX + sliderWidth * rotationFraction, trackY + 2.f}, amber}});
            DrawUtils::setShaderColor(redline.r, redline.g, redline.b, 1.f);
            renderAt(knobMesh, {leftX + sliderWidth * bridgeFraction, trackY}, screenContext, material, stack);
            DrawUtils::setShaderColor(amber.r, amber.g, amber.b, 1.f);
            renderAt(knobMesh, {rightX + sliderWidth * rotationFraction, trackY}, screenContext, material, stack);
            DrawUtils::setShaderColor();
        }

        const glm::vec4 doneBox{settingsPos.x + settingsSize.x - 92.f, sy + 378.f, 70.f, 24.f};
        const bool doneVisible = fullyVisible(doneBox.y, doneBox.w);
        if (doneVisible && clickPending && contains(doneBox, mousePos)) {
            settingsOpen = false;
            clickPending = false;
        }
        if (doneVisible)
            DrawUtils::drawText("<  GARAGE", {doneBox.x + 6.f, doneBox.y + 6.f}, amber, 0.72f);
        screenContext->resetClippingRectangle();
    }

    clickPending = false;
    rightClickPending = false;
    screenContext->resetClippingRectangle();
}

void ClickGui::onKey(const int key, const bool pressed, bool& cancel) {
    if (!g_Client.clickGuiOpened || !pressed)
        return;
    cancel = true;
    if (key == VK_ESCAPE) {
        if (settingsOpen)
            settingsOpen = false;
        else
            setOpen(false);
    }
    rotationDragging = false;
    bridgeDragging = false;
}

void ClickGui::onMouse(const int button, const bool pressed, bool& cancel) {
    if (!g_Client.clickGuiOpened)
        return;
    cancel = true;
    if (!pressed) {
        if (button == 1) {
            rotationDragging = false;
            bridgeDragging = false;
        }
        return;
    }
    if (button == 1)
        clickPending = true;
    else if (button == 2)
        rightClickPending = true;
}

void ClickGui::onWheel(const bool direction, bool& cancel) {
    if (!g_Client.clickGuiOpened)
        return;
    cancel = true;
    wheelDirection = direction ? -1 : 1;
}

void ClickGui::buildMeshes() {
    auto* tessellator = DrawUtils::getTessellator();
    if (tessellator == nullptr)
        return;

    tessellator->begin();
    DrawUtils::addFilledRectangle({0.f, 0.f, lastUiSize.x, lastUiSize.y}, {0.015f, 0.008f, 0.025f, 0.76f}, 0.76f);
    tessellator->end(overlayMesh);

    tessellator->begin();
    DrawUtils::addRoundedOutlinedRectangleBlend(guiX, guiY, guiWidth, guiHeight, meshSmoothness, {0.040f, 0.026f, 0.060f, 0.99f}, {0.42f, 0.23f, 0.62f, 1.f}, 1.2f, 0xF, shellRadius, 0.35f);
    tessellator->end(shellMesh);

    tessellator->begin();
    DrawUtils::addFilledRectangle({guiX + 2.f, guiY + 3.f, guiX + guiWidth - 2.f, guiY + headerHeight - 1.f}, {0.075f, 0.038f, 0.105f, 1.f}, 1.f);
    tessellator->end(headerMesh);

    tessellator->begin();
    DrawUtils::addFilledRectangle({guiX + 2.f, guiY + headerHeight, guiX + sidebarWidth - 1.f, guiY + guiHeight - 3.f}, {0.032f, 0.021f, 0.047f, 0.99f}, 0.99f);
    tessellator->end(sidebarMesh);

    tessellator->begin();
    DrawUtils::addFilledRectangle({guiX + 1.f, guiY + headerHeight - 2.f, guiX + guiWidth - 1.f, guiY + headerHeight}, redline, 1.f);
    DrawUtils::addFilledRectangle({guiX + sidebarWidth - 1.f, guiY + headerHeight, guiX + sidebarWidth, guiY + guiHeight - 1.f}, {0.30f, 0.17f, 0.42f, 1.f}, 1.f);
    for (int tick = 0; tick < 8; ++tick) {
        const float x = guiX + guiWidth - 150.f + tick * 13.f;
        DrawUtils::addFilledRectangle({x, guiY + 1.f, x + 5.f, guiY + 4.f}, tick > 5 ? amber : mce::Color{0.38f, 0.24f, 0.50f, 1.f}, 1.f);
    }
    tessellator->end(redlineMesh);

    tessellator->begin();
    DrawUtils::addOutlinedPartialCircleBlend({0.f, 0.f}, -135.f, 135.f, meshSmoothness, {0.02f, 0.02f, 0.02f, 0.f}, {0.40f, 0.27f, 0.53f, 1.f}, 3.f, 43.f, 0.55f);
    DrawUtils::addOutlinedPartialCircleBlend({0.f, 0.f}, -135.f, 42.f, meshSmoothness, {0.02f, 0.02f, 0.02f, 0.f}, redline, 2.f, 38.f, 0.55f);
    tessellator->end(gaugeMesh);

    tessellator->begin();
    DrawUtils::addRoundedOutlinedRectangleBlend(0.f, 0.f, moduleSize.x, moduleSize.y, meshSmoothness, {0.070f, 0.045f, 0.092f, 0.99f}, {0.38f, 0.23f, 0.51f, 1.f}, 1.f, 0xF, cardRadius, 0.40f);
    tessellator->end(cardMesh);

    tessellator->begin();
    DrawUtils::addRoundedOutlinedRectangleBlend(0.f, 0.f, moduleSize.x, moduleSize.y, meshSmoothness, {0.30f, 0.15f, 0.45f, 0.28f}, {0.68f, 0.42f, 1.f, 0.90f}, 1.f, 0xF, cardRadius, 0.45f);
    tessellator->end(cardHoverMesh);

    tessellator->begin();
    DrawUtils::addRoundedRectangle(0.f, 0.f, 4.f, moduleSize.y, meshSmoothness, redline, 0x5, 2.f);
    DrawUtils::addFilledRectangle({10.f, moduleSize.y - 3.f, moduleSize.x - 10.f, moduleSize.y - 1.f}, {0.48f, 0.17f, 0.82f, 1.f}, 1.f);
    tessellator->end(cardActiveMesh);

    tessellator->begin();
    DrawUtils::addRoundedOutlinedRectangleBlend(0.f, 0.f, settingsSize.x, settingsSize.y, meshSmoothness, {0.052f, 0.033f, 0.072f, 1.f}, {0.36f, 0.21f, 0.49f, 1.f}, 1.f, 0xF, cardRadius, 0.35f);
    tessellator->end(settingsMesh);

    tessellator->begin();
    DrawUtils::addRoundedRectangle(0.f, 0.f, 26.f, 11.f, meshSmoothness, {1.f, 1.f, 1.f, 1.f}, 0xF, 5.5f);
    tessellator->end(toggleTrackMesh);

    tessellator->begin();
    DrawUtils::addCircle({}, meshSmoothness, {1.f, 1.f, 1.f, 1.f}, 4.f);
    tessellator->end(knobMesh);
}
