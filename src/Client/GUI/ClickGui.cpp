#include "ClickGui.h"

#include "../../Client.h"
#include "../../SDK/MC.h"
#include "../../SDK/Render/MeshHelpers.h"
#include "../../Utils/DrawUtils.h"
#include "../Module/ModuleManager.h"
#include "../Module/Modules/BaritoneModule.h"

static float guiXpos = 0.f, guiYpos = 0.f, guiWidth = 0.f, guiHeight = 0.f, scrollOffset = 0.f, renderScrollOffset = 0.f;
static glm::vec2 modulesSectionPos, modulesSectionSize, moduleSize, settingsPanelPos, settingsPanelSize, lastClientUIScreenSize, mousePos;
static int scrollingDirection = 0;
constexpr float smoothness = 30.f;
constexpr float outlineSize = 1.f;
constexpr float radius = 4.2f;
constexpr float panelRadius = 5.f;
constexpr float blend = 0.75f;
static bool shouldClick = false;
static bool shouldRightClick = false;
static bool baritoneSettingsExpanded = false;
static bool rotationSliderDragging = false;
static bool bridgeLengthSliderDragging = false;

void ClickGui::render() {
    const auto screenContext = DrawUtils::getScreenContext();
    const auto tessellator = DrawUtils::getTessellator();
    const auto material = DrawUtils::getUIFillColor();
    const auto guiData = MC::getGuiData();
    const auto clientInstance = MC::getClientInstance();

    if (!g_Client.clickGuiOpened || screenContext == nullptr || tessellator == nullptr ||
        material == nullptr || guiData == nullptr || clientInstance == nullptr) {
        renderScrollOffset = scrollOffset;
        return;
    }

    const auto& clientScreenSize = guiData->screenSizeData.clientScreenSize;
    const auto& clientUIScreenSize = guiData->screenSizeData.clientUIScreenSize;
    if (clientScreenSize.x <= 0.f || clientScreenSize.y <= 0.f ||
        clientUIScreenSize.x <= 0.f || clientUIScreenSize.y <= 0.f)
        return;

    POINT cursor{};
    if (GetCursorPos(&cursor) && ScreenToClient(MC::getWindowHandle(), &cursor))
        mousePos = {static_cast<float>(cursor.x), static_cast<float>(cursor.y)};
    mousePos /= clientScreenSize;
    mousePos *= clientUIScreenSize;

    constexpr float modulePadding = 14.f;
    constexpr int modulesPerRow = 2;
    constexpr int modulesPerCol = 4;

    guiWidth = std::clamp(clientUIScreenSize.x * 0.66f, 540.f, 720.f);
    guiHeight = std::clamp(clientUIScreenSize.y * 0.68f, 370.f, 460.f);
    guiXpos = (clientUIScreenSize.x - guiWidth) / 2.f;
    guiYpos = (clientUIScreenSize.y - guiHeight) / 2.f;
    const float sidebarWidth = std::clamp(guiWidth * 0.225f, 128.f, 154.f);
    constexpr float headerHeight = 46.f;
    modulesSectionPos = {guiXpos + sidebarWidth, guiYpos + headerHeight};
    modulesSectionSize = {guiWidth - sidebarWidth, guiHeight - headerHeight};
    moduleSize = {(modulesSectionSize.x - (modulesPerRow + 1) * modulePadding) / modulesPerRow, 72.f};
    // Settings are a modal view: cover the entire content area so module cards cannot
    // remain visible or receive input behind the panel.
    settingsPanelPos = {guiXpos + modulePadding, guiYpos + headerHeight};
    settingsPanelSize = {guiWidth - modulePadding * 2.f, guiHeight - headerHeight - modulePadding};

    if (!builtMeshes || lastClientUIScreenSize != clientUIScreenSize) {
        lastClientUIScreenSize = clientUIScreenSize;
        buildMeshes();

        builtMeshes = true;
    }

    auto* meshContext = screenContext->toMeshContext();
    if (meshContext == nullptr)
        return;

    overlayMesh.renderMesh(meshContext, material);
    shadowMesh.renderMesh(meshContext, material);
    bgMesh.renderMesh(meshContext, material);
    sidebarMesh.renderMesh(meshContext, material);
    lineMesh.renderMesh(meshContext, material);

    auto& stack = clientInstance->getCamera().worldMatrixStack;
    stack.push();
    stack.top().matrix = translate(stack.top().matrix, {guiXpos + 10.f, guiYpos + 60.f, 0.f});
    navMesh.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());
    stack.pop();

    DrawUtils::drawText("LIMITER", {guiXpos + 15.f, guiYpos + 14.f}, {0.94f, 0.95f, 0.98f, 1.f}, 0.86f);
    DrawUtils::drawText("// LIMITER", {guiXpos + 65.f, guiYpos + 15.f}, {0.63f, 0.36f, 0.98f, 1.f}, 0.68f);
    DrawUtils::drawText("PATHFINDING", {guiXpos + 20.f, guiYpos + 70.f}, {0.79f, 0.72f, 0.98f, 1.f}, 0.7f);
    DrawUtils::drawText("[ TAB ]  Close menu", {guiXpos + 15.f, guiYpos + guiHeight - 24.f}, {0.48f, 0.49f, 0.56f, 1.f}, 0.62f);

    const float headerTextY = guiYpos + 15.f;
    DrawUtils::drawText("Modules", {modulesSectionPos.x + 14.f, headerTextY}, {0.8f, 0.81f, 0.86f, 1.f}, 0.72f);

    const auto baritone = g_modMgr.getModule<BaritoneModule>();
    if (baritone != nullptr) {
        const auto status = baritone->getController().getStatusLine();
        DrawUtils::drawText(status, {guiXpos + guiWidth - DrawUtils::getTextWidth(status, 0.62f) - 14.f, headerTextY}, {0.68f, 0.5f, 0.96f, 1.f}, 0.62f);
    }

    constexpr float speed = 18.f;
    const float scrollUnit = scrollingDirection * speed;

    scrollOffset += scrollUnit;
    scrollingDirection = 0;
    const int modCount = g_modMgr.getModuleCount();
    const float scrollEnd = (moduleSize.y + modulePadding) * (static_cast<float>(modCount) / modulesPerRow - modulesPerCol + 1.f);

    if (modCount > modulesPerCol * modulesPerRow)
        scrollOffset = std::clamp(scrollOffset, 0.f, scrollEnd + 1.f);
    else
        scrollOffset = 0.f;

    renderScrollOffset += (scrollOffset - renderScrollOffset) *
        std::clamp(static_cast<float>(g_Client.deltaTime) * 14.f, 0.f, 1.f);

    int row = 0, column = 0;

    DrawUtils::getScreenContext()->setClippingRectangle(modulesSectionPos.x, modulesSectionPos.y + 0.5f,
        modulesSectionSize.x, modulesSectionSize.y - 0.5f);

    for (auto& mod : g_modMgr.getSortedModules()) {
        if (baritoneSettingsExpanded)
            continue;

        float posX = modulesSectionPos.x + modulePadding + column * (moduleSize.x + modulePadding);

        if (column >= modulesPerRow) {
            row++;
            column = 0;
            posX = modulesSectionPos.x + modulePadding + column * (moduleSize.x + modulePadding);
        }

        column++;

        float posY = modulesSectionPos.y + modulePadding + row * (moduleSize.y + modulePadding) - renderScrollOffset;

        if (shouldRightClick && mousePos.x >= posX && mousePos.x < posX + moduleSize.x &&
            mousePos.y >= posY && mousePos.y < posY + moduleSize.y) {
            if (baritone != nullptr && mod.get() == baritone) {
                baritoneSettingsExpanded = !baritoneSettingsExpanded;
            }
            shouldRightClick = false;
        }

        if (posY >= guiYpos + guiHeight || posY + moduleSize.y <= modulesSectionPos.y) {
            if (mod->isEnabled())
                mod->enabledButtonRegion = 1.f;
            else
                mod->enabledButtonRegion = 0.f;
            continue;
        }

        if (mod->isEnabled()) {
            if (mod->enabledButtonRegion < 1.f) {
                mod->enabledButtonRegion += 5.f * g_Client.deltaTime;

                if (mod->enabledButtonRegion >= 1.f)
                    mod->enabledButtonRegion = 1.f;
            }
        }
        else {
            if (mod->enabledButtonRegion > 0.f) {
                mod->enabledButtonRegion -= 5.f * g_Client.deltaTime;

                if (mod->enabledButtonRegion <= 0.f)
                    mod->enabledButtonRegion = 0.f;
            }
        }

        const bool cardHovered = mousePos.x >= posX && mousePos.x < posX + moduleSize.x &&
            mousePos.y >= posY && mousePos.y < posY + moduleSize.y;

        {
            stack.push();

            auto& matrix = stack.top().matrix;

            matrix = translate(matrix, {posX, posY, 0.f});

            modMesh.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());

            stack.pop();
        }
        if (cardHovered) {
            stack.push();
            stack.top().matrix = translate(stack.top().matrix, {posX, posY, 0.f});
            modHoverMesh.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());
            stack.pop();
        }
        if (mod->enabledButtonRegion > 0.f) {
            DrawUtils::setShaderColor(0.63f, 0.34f, 0.98f, mod->enabledButtonRegion);
            stack.push();
            stack.top().matrix = translate(stack.top().matrix, {posX + 1.f, posY + 14.f, 0.f});
            cardAccentMesh.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());
            stack.pop();
            DrawUtils::setShaderColor();
        }

        {
            DrawUtils::setShaderColor(
                0.2f + 0.42f * mod->enabledButtonRegion,
                0.21f + 0.13f * mod->enabledButtonRegion,
                0.27f + 0.71f * mod->enabledButtonRegion, 1.f);

            const glm::vec4 pos{posX + moduleSize.x - 39.f, posY + moduleSize.y - 23.f, 25.f, 10.f};

            stack.push();

            auto& matrix = stack.top().matrix;

            matrix = translate(matrix, {pos.x, pos.y, 0.f});

            enabledStateMesh.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());

            stack.pop();

            DrawUtils::setShaderColor();

            if (shouldClick && mousePos.x >= pos.x && mousePos.x < pos.x + pos.z && mousePos.y >= pos.y && mousePos.y < pos.y + pos.w) {
                shouldClick = false;
                mod->toggle();
            }
        }

        {
            stack.push();

            auto& matrix = stack.top().matrix;

            matrix = translate(matrix, {posX + moduleSize.x - 39.f + radius + (10.f - radius * 2.f) +
                (25.f - radius * 2.f - (10.f - radius * 2.f) * 2.f) * mod->enabledButtonRegion,
                posY + moduleSize.y - 18.f, 0.f});

            circle.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());

            stack.pop();
        }

        const glm::vec2 textPos = {
            posX + 15.f,
            posY + 14.f
        };

        if (textPos.y + DrawUtils::getFontHeight() >= guiYpos && textPos.y <= guiYpos + guiHeight)
            DrawUtils::drawText(mod->getName(), textPos, {0.9f, 0.95f, 0.98f, 1.f}, 0.9f);

        if (baritone != nullptr && mod.get() == baritone) {
            DrawUtils::drawText("Autonomous navigation", {posX + 15.f, posY + 35.f},
                {0.44f, 0.54f, 0.63f, 1.f}, 0.62f);
            DrawUtils::drawText("Right click for settings", {posX + 15.f, posY + moduleSize.y - 21.f},
                {0.64f, 0.48f, 0.92f, 1.f}, 0.56f);
        }
    }

    if (baritone != nullptr && baritoneSettingsExpanded) {
        DrawUtils::getScreenContext()->resetClippingRectangle();
        stack.push();
        stack.top().matrix = translate(stack.top().matrix, {settingsPanelPos.x, settingsPanelPos.y, 0.f});
        settingsMesh.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());
        stack.pop();

        auto& controller = baritone->getController();
        auto& pathOptions = controller.getOptions();
        auto& executionOptions = controller.getExecutionOptions();
        auto& renderOptions = controller.getRenderOptions();

        struct Toggle {
            const char* name;
            bool* value;
        };

        const std::array movementToggles{
            Toggle{"Diagonal", &pathOptions.allowDiagonal},
            Toggle{"Water", &pathOptions.allowWater},
            Toggle{"Step Up", &pathOptions.allowAscend},
            Toggle{"Drops", &pathOptions.allowFall},
            Toggle{"Parkour", &pathOptions.allowParkour},
            Toggle{"Sprint", &executionOptions.sprint},
            Toggle{"Auto Replan", &controller.getReplanWhenStuck()},
            Toggle{"Build Bridges", &pathOptions.allowBridge},
        };
        const std::array visualToggles{
            Toggle{"Render Path", &renderOptions.renderPath},
            Toggle{"Render Goal", &renderOptions.renderGoal},
            Toggle{"Animated Goal", &renderOptions.animatedGoal},
            Toggle{"Calc Details", &renderOptions.renderCalculations},
            Toggle{"Through Walls", &renderOptions.renderThroughWalls}
        };

        DrawUtils::drawText("Limiter Settings", {settingsPanelPos.x + 18.f, settingsPanelPos.y + 13.f},
            {0.92f, 0.97f, 1.f, 1.f}, 1.0f);
        DrawUtils::drawText("Tune movement behavior and route visualization",
            {settingsPanelPos.x + 18.f, settingsPanelPos.y + 31.f}, {0.4f, 0.52f, 0.62f, 1.f}, 0.62f);

        constexpr float settingsTop = 54.f;
        constexpr float headingGap = 18.f;
        constexpr float rowHeight = 20.f;
        const float columnGap = 24.f;
        const float columnWidth = (settingsPanelSize.x - 36.f - columnGap) / 2.f;

        auto drawColumn = [&](const char* heading, const auto& toggles, const float x) {
            DrawUtils::drawText(heading, {x, settingsPanelPos.y + settingsTop}, {0.65f, 0.4f, 0.98f, 1.f}, 0.78f);

            for (std::size_t index = 0; index < toggles.size(); ++index) {
                const auto& toggle = toggles[index];
                const float rowY = settingsPanelPos.y + settingsTop + headingGap + static_cast<float>(index) * rowHeight;
                const glm::vec4 hitbox{x - 3.f, rowY - 3.f, columnWidth, rowHeight - 1.f};

                if (shouldClick && mousePos.x >= hitbox.x && mousePos.x < hitbox.x + hitbox.z &&
                    mousePos.y >= hitbox.y && mousePos.y < hitbox.y + hitbox.w) {
                    *toggle.value = !*toggle.value;
                    shouldClick = false;
                }

                DrawUtils::drawText(toggle.name, {x, rowY}, {0.78f, 0.84f, 0.9f, 1.f}, 0.76f);

                const float switchX = x + columnWidth - 25.f;
                DrawUtils::setShaderColor(*toggle.value ? 0.62f : 0.2f,
                    *toggle.value ? 0.34f : 0.21f, *toggle.value ? 0.98f : 0.27f, 1.f);
                stack.push();
                stack.top().matrix = translate(stack.top().matrix, {switchX, rowY - 1.f, 0.f});
                enabledStateMesh.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());
                stack.pop();

                DrawUtils::setShaderColor();
                stack.push();
                const float knobX = switchX + radius + (10.f - radius * 2.f) +
                    (25.f - radius * 2.f - (10.f - radius * 2.f) * 2.f) * (*toggle.value ? 1.f : 0.f);
                stack.top().matrix = translate(stack.top().matrix, {knobX, rowY + 4.f, 0.f});
                circle.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());
                stack.pop();
            }
        };

        drawColumn("MOVEMENT", movementToggles, settingsPanelPos.x + 14.f);
        drawColumn("VISUALS", visualToggles, settingsPanelPos.x + 14.f + columnWidth + columnGap);

        constexpr float minimumSmoothness = 0.25f;
        constexpr float maximumSmoothness = 10.f;
        constexpr int minimumBridgeLength = 1;
        constexpr int maximumBridgeLength = 16;
        constexpr float sliderGap = 24.f;
        const float controlsX = settingsPanelPos.x + 18.f;
        const float sliderWidth = (settingsPanelSize.x - 36.f - sliderGap) / 2.f;
        const float bridgeSliderX = controlsX;
        const float rotationSliderX = controlsX + sliderWidth + sliderGap;
        const float sliderY = settingsPanelPos.y + settingsTop + headingGap +
            static_cast<float>(std::max(movementToggles.size(), visualToggles.size())) * rowHeight + 18.f;
        const glm::vec4 bridgeSliderHitbox{bridgeSliderX - 4.f, sliderY + 12.f, sliderWidth + 8.f, 20.f};
        const glm::vec4 rotationSliderHitbox{rotationSliderX - 4.f, sliderY + 12.f, sliderWidth + 8.f, 20.f};

        if (shouldClick && mousePos.x >= bridgeSliderHitbox.x && mousePos.x < bridgeSliderHitbox.x + bridgeSliderHitbox.z &&
            mousePos.y >= bridgeSliderHitbox.y && mousePos.y < bridgeSliderHitbox.y + bridgeSliderHitbox.w) {
            bridgeLengthSliderDragging = true;
            shouldClick = false;
        } else if (shouldClick && mousePos.x >= rotationSliderHitbox.x && mousePos.x < rotationSliderHitbox.x + rotationSliderHitbox.z &&
            mousePos.y >= rotationSliderHitbox.y && mousePos.y < rotationSliderHitbox.y + rotationSliderHitbox.w) {
            rotationSliderDragging = true;
            shouldClick = false;
        }
        if (bridgeLengthSliderDragging) {
            const float fraction = std::clamp((mousePos.x - bridgeSliderX) / sliderWidth, 0.f, 1.f);
            pathOptions.maxBridgeLength = std::clamp(
                static_cast<int>(std::round(minimumBridgeLength + fraction *
                    static_cast<float>(maximumBridgeLength - minimumBridgeLength))),
                minimumBridgeLength, maximumBridgeLength);
        }
        if (rotationSliderDragging) {
            const float fraction = std::clamp((mousePos.x - rotationSliderX) / sliderWidth, 0.f, 1.f);
            executionOptions.rotationSmoothness = minimumSmoothness + fraction * (maximumSmoothness - minimumSmoothness);
        }

        const auto bridgeLabel = std::format("Max Bridge Gap: {} blocks", pathOptions.maxBridgeLength);
        const auto smoothnessLabel = std::format("Rotation Speed: {:.2f}x", executionOptions.rotationSmoothness);
        DrawUtils::drawText(bridgeLabel, {bridgeSliderX, sliderY}, {0.88f, 0.9f, 0.94f, 1.f}, 0.72f);
        DrawUtils::drawText(smoothnessLabel, {rotationSliderX, sliderY}, {0.88f, 0.9f, 0.94f, 1.f}, 0.72f);
        const float trackY = sliderY + 21.f;
        const float bridgeFraction = static_cast<float>(pathOptions.maxBridgeLength - minimumBridgeLength) /
            static_cast<float>(maximumBridgeLength - minimumBridgeLength);
        const float rotationFraction = (executionOptions.rotationSmoothness - minimumSmoothness) /
            (maximumSmoothness - minimumSmoothness);

        auto* sliderTessellator = DrawUtils::getTessellator();
        sliderTessellator->begin();
        DrawUtils::addFilledRectangle({bridgeSliderX, trackY - 1.5f, bridgeSliderX + sliderWidth, trackY + 1.5f},
            {0.18f, 0.22f, 0.28f, 1.f}, 1.f);
        DrawUtils::addFilledRectangle({bridgeSliderX, trackY - 1.5f, bridgeSliderX + sliderWidth * bridgeFraction, trackY + 1.5f},
            {0.62f, 0.34f, 0.98f, 1.f}, 1.f);
        DrawUtils::addFilledRectangle({rotationSliderX, trackY - 1.5f, rotationSliderX + sliderWidth, trackY + 1.5f},
            {0.18f, 0.22f, 0.28f, 1.f}, 1.f);
        DrawUtils::addFilledRectangle({rotationSliderX, trackY - 1.5f, rotationSliderX + sliderWidth * rotationFraction, trackY + 1.5f},
            {0.62f, 0.34f, 0.98f, 1.f}, 1.f);
        MeshHelpers::renderMeshImmediately(DrawUtils::getScreenContext(), sliderTessellator, DrawUtils::getUIFillColor());

        DrawUtils::setShaderColor(0.62f, 0.34f, 0.98f, 1.f);
        stack.push();
        stack.top().matrix = translate(stack.top().matrix, {bridgeSliderX + sliderWidth * bridgeFraction, trackY, 0.f});
        circle.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());
        stack.pop();
        stack.push();
        stack.top().matrix = translate(stack.top().matrix, {rotationSliderX + sliderWidth * rotationFraction, trackY, 0.f});
        circle.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());
        stack.pop();
        DrawUtils::setShaderColor();

        const std::string closeLabel = "Done  ESC";
        const float closeX = settingsPanelPos.x + settingsPanelSize.x - DrawUtils::getTextWidth(closeLabel, 0.8f) - 16.f;
        const float closeY = settingsPanelPos.y + settingsPanelSize.y - 28.f;
        const glm::vec4 closeHitbox{closeX - 8.f, closeY - 5.f, DrawUtils::getTextWidth(closeLabel, 0.8f) + 16.f, 22.f};
        if (shouldClick && mousePos.x >= closeHitbox.x && mousePos.x < closeHitbox.x + closeHitbox.z &&
            mousePos.y >= closeHitbox.y && mousePos.y < closeHitbox.y + closeHitbox.w) {
            baritoneSettingsExpanded = false;
            shouldClick = false;
        }
        DrawUtils::drawText(closeLabel, {closeX, closeY}, {0.68f, 0.46f, 1.f, 1.f}, 0.8f);
    }

    shouldRightClick = false;

    DrawUtils::getScreenContext()->resetClippingRectangle();

    /*const auto tess = DrawUtils::getTessellator();

    tess->begin();

    DrawUtils::addRoundedOutlinedRectangleBlendChroma(25.f, 25.f, 50.f, 50.f, smoothness, 0.45f, 1.f, outlineSize, 0xF, radius, blend);

    MeshHelpers::renderMeshImmediately(DrawUtils::getScreenContext(), DrawUtils::getTessellator(), DrawUtils::getUIFillColor());*/
}

void ClickGui::onKey(int key, bool pressed, bool& cancel) {
    if (!g_Client.clickGuiOpened || !pressed)
        return;

    cancel = true;

    if (key == VK_ESCAPE) {
        if (baritoneSettingsExpanded)
            baritoneSettingsExpanded = false;
        else {
            g_Client.clickGuiOpened = false;
            MC::getMinecraftGame()->grabMouse();
        }
    }
    rotationSliderDragging = false;
    bridgeLengthSliderDragging = false;
}

void ClickGui::onMouse(const int button, const bool pressed, bool& cancel) {
    if (!g_Client.clickGuiOpened)
        return;

    cancel = true;

    if (button == 1 && !pressed) {
        rotationSliderDragging = false;
        bridgeLengthSliderDragging = false;
        return;
    }
    if (!pressed)
        return;

    if (button == 1)
        shouldClick = true;
    else if (button == 2)
        shouldRightClick = true;
}

void ClickGui::onWheel(const bool direction, bool& cancel) {
    if (!g_Client.clickGuiOpened)
        return;

    cancel = true;

    scrollingDirection = direction ? -1 : 1;
}

void ClickGui::buildMeshes() {
    const auto tess = DrawUtils::getTessellator();
    if (tess == nullptr)
        return;

    { // Dimmed backdrop. This is deliberately material-safe faux blur.
        tess->begin();
        DrawUtils::addFilledRectangle({0.f, 0.f, lastClientUIScreenSize.x, lastClientUIScreenSize.y},
            {0.015f, 0.012f, 0.025f}, 0.16f);
        tess->end(overlayMesh);
    }

    { // Soft outer shadow
        tess->begin();
        DrawUtils::addRoundedRectangle(guiXpos - 5.f, guiYpos + 3.f, guiWidth + 10.f, guiHeight + 8.f,
            smoothness, {0.f, 0.f, 0.f, 0.28f}, 0xF, panelRadius + 2.f);
        tess->end(shadowMesh);
    }

    { // Frosted main surface
        tess->begin();
        DrawUtils::addRoundedOutlinedRectangleBlend(guiXpos, guiYpos, guiWidth, guiHeight, smoothness,
            {0.035f, 0.032f, 0.052f, 0.74f}, {0.32f, 0.2f, 0.5f, 0.95f},
            0.8f, 0xF, panelRadius, 0.62f);
        tess->end(bgMesh);
    }

    { // Sidebar glass
        tess->begin();
        const float sidebarWidth = modulesSectionPos.x - guiXpos;
        DrawUtils::addRoundedRectangle(guiXpos + 1.f, guiYpos + 1.f, sidebarWidth - 1.f, guiHeight - 2.f,
            smoothness, {0.025f, 0.022f, 0.04f, 0.66f}, 0xA, panelRadius - 1.f);
        tess->end(sidebarMesh);
    }

    { // Selected navigation pill (local coordinates)
        tess->begin();
        const float sidebarWidth = modulesSectionPos.x - guiXpos;
        DrawUtils::addRoundedRectangle(0.f, 0.f, sidebarWidth - 20.f, 31.f, smoothness,
            {0.2f, 0.1f, 0.32f, 0.72f}, 0xF, 3.f);
        tess->end(navMesh);
    }

    { // Hairline separators
        tess->begin();
        DrawUtils::addFilledRectangle({guiXpos + 1.f, guiYpos + 1.f,
            guiXpos + guiWidth - 1.f, guiYpos + 3.f}, {0.62f, 0.34f, 0.98f}, 0.95f);
        DrawUtils::addFilledRectangle({modulesSectionPos.x - 0.5f, guiYpos + 1.f,
            modulesSectionPos.x + 0.5f, guiYpos + guiHeight - 1.f}, {0.3f, 0.22f, 0.4f}, 0.7f);
        DrawUtils::addFilledRectangle({modulesSectionPos.x, modulesSectionPos.y - 0.5f,
            guiXpos + guiWidth - 1.f, modulesSectionPos.y + 0.5f}, {0.3f, 0.22f, 0.4f}, 0.55f);
        tess->end(lineMesh);
    }

    { // Module card
        tess->begin();
        DrawUtils::addRoundedOutlinedRectangleBlend(0.f, 0.f, moduleSize.x, moduleSize.y, smoothness,
            {0.045f, 0.042f, 0.062f, 0.62f}, {0.25f, 0.2f, 0.34f, 0.88f},
            0.75f, 0xF, 3.f, 0.58f);
        tess->end(modMesh);
    }

    { // Hover sheen
        tess->begin();
        DrawUtils::addRoundedOutlinedRectangleBlend(0.f, 0.f, moduleSize.x, moduleSize.y, smoothness,
            {0.11f, 0.07f, 0.16f, 0.25f}, {0.58f, 0.34f, 0.9f, 0.72f},
            0.65f, 0xF, 3.f, 0.58f);
        tess->end(modHoverMesh);
    }

    { // Enabled accent rail
        tess->begin();
        DrawUtils::addRoundedRectangle(0.f, 0.f, 3.f, 34.f, smoothness,
            {1.f, 1.f, 1.f, 1.f}, 0xF, 1.5f);
        tess->end(cardAccentMesh);
    }

    { // Settings panel
        tess->begin();
        DrawUtils::addRoundedOutlinedRectangleBlend(0.f, 0.f, settingsPanelSize.x, settingsPanelSize.y, smoothness,
            {0.035f, 0.032f, 0.052f, 0.82f}, {0.48f, 0.28f, 0.76f, 0.82f},
            0.8f, 0xF, 4.f, 0.62f);
        tess->end(settingsMesh);
    }

    { // Enabled button state background
        tess->begin();

        DrawUtils::addRoundedRectangle(0.f, 0.f, 25.f, 10.f, smoothness, {}, 0xF, radius);

        tess->end(enabledStateMesh);
    }

    { // Enabled button
        tess->begin();

        DrawUtils::addCircle({}, smoothness, {}, radius);

        tess->end(circle);
    }
}
