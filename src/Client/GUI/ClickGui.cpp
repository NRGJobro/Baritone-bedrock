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
constexpr float blend = 0.75f;
static bool shouldClick = false;
static bool shouldRightClick = false;
static bool baritoneSettingsExpanded = false;

void ClickGui::render() {
    if (!g_Client.clickGuiOpened || DrawUtils::getTessellator() == nullptr) {
        renderScrollOffset = scrollOffset;
        return;
    }

    const auto& clientScreenSize = MC::getGuiData()->screenSizeData.clientScreenSize;
    const auto& clientUIScreenSize = MC::getGuiData()->screenSizeData.clientUIScreenSize;

    mousePos = MC::getClientInstance()->getMousePos();
    mousePos /= clientScreenSize;
    mousePos *= clientUIScreenSize;

    constexpr float modulePadding = 10.f;
    constexpr int modulesPerRow = 4;
    constexpr int modulesPerCol = 3;

    guiWidth = clientUIScreenSize.x * 0.75f;
    guiHeight = clientUIScreenSize.y * 0.75f;
    guiXpos = (clientUIScreenSize.x - guiWidth) / 2.f;
    guiYpos = (clientUIScreenSize.y - guiHeight) / 2.f;
    modulesSectionPos = {guiXpos + guiWidth / 5.f, guiYpos + guiHeight / 10.f};
    modulesSectionSize = {guiXpos + guiWidth - modulesSectionPos.x, guiYpos + guiHeight - modulesSectionPos.y};
    moduleSize = {(modulesSectionSize.x - (modulesPerRow + 1) * 10.f) / modulesPerRow, (modulesSectionSize.y - (modulesPerCol + 1) * 10.f) / modulesPerCol};
    settingsPanelPos = {modulesSectionPos.x + modulePadding * 2.f + moduleSize.x, modulesSectionPos.y + modulePadding};
    settingsPanelSize = {modulesSectionSize.x - moduleSize.x - modulePadding * 3.f, modulesSectionSize.y - modulePadding * 2.f};

    if (!builtMeshes || lastClientUIScreenSize != clientUIScreenSize) {
        buildMeshes();

        builtMeshes = true;
        lastClientUIScreenSize = clientUIScreenSize;
    }

    bgMesh.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());
    lineMesh.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());

    const std::string title = "Borion Baritone";
    const float headerTextY = guiYpos + (modulesSectionPos.y - guiYpos - DrawUtils::getFontHeight()) / 2.f;
    DrawUtils::drawText(title, {guiXpos + 12.f, headerTextY}, {0.85f, 0.95f, 1.f, 1.f});

    const auto baritone = g_modMgr.getModule<BaritoneModule>();
    if (baritone != nullptr) {
        const auto status = baritone->getController().getStatusLine();
        DrawUtils::drawText(status, {guiXpos + guiWidth - DrawUtils::getTextWidth(status, 0.75f) - 12.f, headerTextY}, {0.7f, 0.75f, 0.8f, 1.f}, 0.75f);
    }

    constexpr float speed = 15.f;
    constexpr float modSpeed = speed * speed / 2.f;
    const float scrollUnit = scrollingDirection * speed;

    scrollOffset += scrollUnit;
    scrollingDirection = 0;
    const int modCount = g_modMgr.getModuleCount();
    const float scrollEnd = (moduleSize.y + modulePadding) * (static_cast<float>(modCount) / modulesPerRow - modulesPerCol + 1.f);

    if (modCount > modulesPerCol * modulesPerRow)
        scrollOffset = std::clamp(scrollOffset, 0.f, scrollEnd + 1.f);
    else
        scrollOffset = 0.f;

    if (renderScrollOffset != scrollOffset) {
        const float diff = std::abs(renderScrollOffset - scrollOffset);
        const float s = std::clamp(modSpeed * (diff / scrollUnit), modSpeed, modSpeed * 3.f);

        if (renderScrollOffset > scrollOffset) {
            renderScrollOffset -= static_cast<float>(s * g_Client.deltaTime);

            if (renderScrollOffset < scrollOffset)
                renderScrollOffset = scrollOffset;
        }
        else {
            renderScrollOffset += static_cast<float>(s * g_Client.deltaTime);

            if (renderScrollOffset > scrollOffset)
                renderScrollOffset = scrollOffset;
        }
    }

    auto& stack = MC::getClientInstance()->getCamera().worldMatrixStack;

    int row = 0, column = 0;

    DrawUtils::getScreenContext()->setClippingRectangle(guiXpos, modulesSectionPos.y + 0.5f, guiWidth, guiHeight - (modulesSectionPos.y + 0.5f - guiYpos));

    for (auto& mod : g_modMgr.getSortedModules()) {
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
            if (baritone != nullptr && mod.get() == baritone)
                baritoneSettingsExpanded = !baritoneSettingsExpanded;
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

        {
            stack.push();

            auto& matrix = stack.top().matrix;

            matrix = translate(matrix, {posX, posY, 0.f});

            modMesh.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());

            stack.pop();
        }

        {
            DrawUtils::setShaderColor(1.f * (1.f - mod->enabledButtonRegion), 1.f * mod->enabledButtonRegion, 0.f, 1.f);

            const glm::vec4 pos{posX + moduleSize.x / 2.f - 12.5f, posY + moduleSize.y - 15.f, 25.f, 10.f};

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

            matrix = translate(matrix, {posX + moduleSize.x / 2.f - 12.5f + radius + (10.f - radius * 2.f) +
                (25.f - radius * 2.f - (10.f - radius * 2.f) * 2.f) * mod->enabledButtonRegion, posY + moduleSize.y - 10.f, 0.f});

            circle.renderMesh(DrawUtils::getScreenContext()->toMeshContext(), DrawUtils::getUIFillColor());

            stack.pop();
        }

        const glm::vec2 textPos = {
            posX + moduleSize.x / 2.f - DrawUtils::getTextWidth(mod->getName()) / 2.f,
            posY + 15.f
        };

        if (textPos.y + DrawUtils::getFontHeight() >= guiYpos && textPos.y <= guiYpos + guiHeight)
            DrawUtils::drawText(mod->getName(), textPos);

        if (baritone != nullptr && mod.get() == baritone) {
            const std::string hint = baritoneSettingsExpanded ? "Right click: close" : "Right click: settings";
            DrawUtils::drawText(hint, {posX + moduleSize.x / 2.f - DrawUtils::getTextWidth(hint, 0.5f) / 2.f, posY + 31.f},
                {0.65f, 0.7f, 0.76f, 1.f}, 0.5f);
        }
    }

    if (baritone != nullptr && baritoneSettingsExpanded) {
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
            Toggle{"Sprint", &executionOptions.sprint},
            Toggle{"Auto Replan", &controller.getReplanWhenStuck()}
        };
        const std::array visualToggles{
            Toggle{"Render Path", &renderOptions.renderPath},
            Toggle{"Render Goal", &renderOptions.renderGoal},
            Toggle{"Animated Goal", &renderOptions.animatedGoal},
            Toggle{"Fade Path", &renderOptions.fadePath},
            Toggle{"Line Only", &renderOptions.pathAsLine},
            Toggle{"Calc Details", &renderOptions.renderCalculations}
        };

        DrawUtils::drawText("Baritone Settings", {settingsPanelPos.x + 14.f, settingsPanelPos.y + 12.f}, {0.85f, 0.95f, 1.f, 1.f}, 0.85f);

        constexpr float settingsTop = 40.f;
        constexpr float headingGap = 18.f;
        constexpr float rowHeight = 20.f;
        const float columnGap = 24.f;
        const float columnWidth = (settingsPanelSize.x - 36.f - columnGap) / 2.f;

        auto drawColumn = [&](const char* heading, const auto& toggles, const float x) {
            DrawUtils::drawText(heading, {x, settingsPanelPos.y + settingsTop}, {0.35f, 0.8f, 1.f, 1.f}, 0.8f);

            for (std::size_t index = 0; index < toggles.size(); ++index) {
                const auto& toggle = toggles[index];
                const float rowY = settingsPanelPos.y + settingsTop + headingGap + static_cast<float>(index) * rowHeight;
                const glm::vec4 hitbox{x - 3.f, rowY - 3.f, columnWidth, rowHeight - 1.f};

                if (shouldClick && mousePos.x >= hitbox.x && mousePos.x < hitbox.x + hitbox.z &&
                    mousePos.y >= hitbox.y && mousePos.y < hitbox.y + hitbox.w) {
                    *toggle.value = !*toggle.value;
                    shouldClick = false;
                }

                DrawUtils::drawText(toggle.name, {x, rowY}, {0.88f, 0.9f, 0.94f, 1.f}, 0.7f);

                const float switchX = x + columnWidth - 25.f;
                DrawUtils::setShaderColor(*toggle.value ? 0.1f : 0.75f, *toggle.value ? 0.8f : 0.12f, 0.12f, 1.f);
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
        g_Client.clickGuiOpened = false;
        MC::getMinecraftGame()->grabMouse();
    }
}

void ClickGui::onMouse(const int button, const bool pressed, bool& cancel) {
    if (!g_Client.clickGuiOpened || !pressed)
        return;

    cancel = true;

    if (button == 1)
        shouldClick = pressed;
    else if (button == 2)
        shouldRightClick = pressed;
}

void ClickGui::onWheel(const bool direction, bool& cancel) {
    if (!g_Client.clickGuiOpened)
        return;

    cancel = true;

    scrollingDirection = direction ? -1 : 1;
}

void ClickGui::buildMeshes() {
    const auto tess = DrawUtils::getTessellator();

    { // Background
        tess->begin();

        DrawUtils::addRoundedOutlinedRectangleBlend(guiXpos - 1.f, guiYpos - 1.f, guiWidth + 1.f, guiHeight + 1.f, smoothness,
            {0.f, 0.f, 0.f, 0.65f}, {0.f, 0.f, 0.f, 0.85f}, outlineSize, 0xF, radius, blend);

        tess->end(bgMesh);
    }

    { // Line
        tess->begin();

        DrawUtils::addFilledRectangle({guiXpos, modulesSectionPos.y - 0.5f, guiXpos + guiWidth, modulesSectionPos.y + 0.5f},
            {0.f, 0.f, 0.f}, 0.85f);

        tess->end(lineMesh);
    }

    { // Module
        tess->begin();

        DrawUtils::addRoundedOutlinedRectangleBlend(0.f, 0.f, moduleSize.x, moduleSize.y, smoothness,
            {1.f, 1.f, 1.f, 0.15f}, {1.f, 1.f, 1.f, 0.45f}, outlineSize, 0xF, radius, blend);

        tess->end(modMesh);
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
