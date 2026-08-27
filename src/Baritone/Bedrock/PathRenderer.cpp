#include "PathRenderer.h"

#include "../BaritoneController.h"
#include "../../SDK/MC.h"
#include "../../SDK/Render/MeshHelpers.h"
#include "../../Utils/DrawUtils.h"
#include "../../Utils/TimeUtils.h"

#include <cmath>
#include <limits>
#include <numbers>

namespace {

void vertex(Tessellator* tessellator, const glm::vec3& origin, const float x, const float y, const float z) {
    tessellator->vertex(glm::vec3{x, y, z} - origin);
}

void line(Tessellator* tessellator, const glm::vec3& origin, const glm::vec3& from, const glm::vec3& to,
    const mce::Color* color = nullptr) {
    if (color != nullptr)
        tessellator->color(*color);
    vertex(tessellator, origin, from.x, from.y, from.z);
    if (color != nullptr)
        tessellator->color(*color);
    vertex(tessellator, origin, to.x, to.y, to.z);
}

void pathSegment(Tessellator* tessellator, const glm::vec3& origin, const baritone::BlockPos& from,
    const baritone::BlockPos& to, const bool lineOnly) {
    constexpr float offset = 0.5f;
    constexpr float extraOffset = offset + 0.03f;
    const glm::vec3 first{from.x + offset, from.y + offset, from.z + offset};
    const glm::vec3 second{to.x + offset, to.y + offset, to.z + offset};

    line(tessellator, origin, first, second);
    if (lineOnly)
        return;

    const auto firstTop = glm::vec3{first.x, from.y + extraOffset, first.z};
    const auto secondTop = glm::vec3{second.x, to.y + extraOffset, second.z};
    line(tessellator, origin, second, secondTop);
    line(tessellator, origin, secondTop, firstTop);
    line(tessellator, origin, firstTop, first);
}

void horizontalQuad(Tessellator* tessellator, const glm::vec3& origin, const float minX, const float maxX,
    const float minZ, const float maxZ, const float y) {
    if (y == 0.f)
        return;
    line(tessellator, origin, {minX, y, minZ}, {maxX, y, minZ});
    line(tessellator, origin, {maxX, y, minZ}, {maxX, y, maxZ});
    line(tessellator, origin, {maxX, y, maxZ}, {minX, y, maxZ});
    line(tessellator, origin, {minX, y, maxZ}, {minX, y, minZ});
}

void goalBox(Tessellator* tessellator, const glm::vec3& origin, const float minX, const float maxX,
    const float minZ, const float maxZ, const float minY, const float maxY, const float y1, const float y2) {
    horizontalQuad(tessellator, origin, minX, maxX, minZ, maxZ, y1);
    horizontalQuad(tessellator, origin, minX, maxX, minZ, maxZ, y2);

    for (float y = minY; y < maxY; y += 16.f) {
        const float top = std::min(maxY, y + 16.f);
        line(tessellator, origin, {minX, y, minZ}, {minX, top, minZ});
        line(tessellator, origin, {maxX, y, minZ}, {maxX, top, minZ});
        line(tessellator, origin, {maxX, y, maxZ}, {maxX, top, maxZ});
        line(tessellator, origin, {minX, y, maxZ}, {minX, top, maxZ});
    }
}

void renderGoal(Tessellator* tessellator, const glm::vec3& origin, const baritone::Goal* goal, const bool animated) {
    if (goal == nullptr)
        return;

    const float pulse = animated ? std::cos(static_cast<float>(TimeUtils::currentTimeMillis() % 2000) / 2000.f *
        2.f * std::numbers::pi_v<float>) : 0.999f;

    auto renderPositionGoal = [&](const baritone::BlockPos& pos) {
        constexpr float inset = 0.002f;
        goalBox(tessellator, origin, pos.x + inset, pos.x + 1.f - inset, pos.z + inset, pos.z + 1.f - inset,
            static_cast<float>(pos.y), pos.y + 2.f, pos.y + 1.f + pulse, pos.y + 1.f - pulse);
    };

    if (const auto block = dynamic_cast<const baritone::GoalBlock*>(goal)) {
        renderPositionGoal(block->getTarget());
    } else if (const auto nearGoal = dynamic_cast<const baritone::GoalNear*>(goal)) {
        renderPositionGoal(nearGoal->getTarget());
    } else if (const auto xzGoal = dynamic_cast<const baritone::GoalXZ*>(goal)) {
        constexpr float inset = 0.002f;
        goalBox(tessellator, origin, xzGoal->getX() + inset, xzGoal->getX() + 1.f - inset,
            xzGoal->getZ() + inset, xzGoal->getZ() + 1.f - inset, -64.f, 320.f, 0.f, 0.f);
    } else if (const auto yGoal = dynamic_cast<const baritone::GoalYLevel*>(goal)) {
        const auto player = MC::getLocalPlayer();
        if (player == nullptr)
            return;
        const auto feet = player->getFeetPosition();
        const float y = static_cast<float>(yGoal->getY());
        goalBox(tessellator, origin, feet.x - 15.f, feet.x + 15.f, feet.z - 15.f, feet.z + 15.f,
            y, y + 2.f, y + 1.f + pulse, y + 1.f - pulse);
    }
}

void renderRoute(Tessellator* tessellator, const glm::vec3& origin, const std::vector<baritone::PathNode>& path,
    const std::size_t renderBegin, const bool lineOnly, const bool fade, const mce::Color& color,
    const std::size_t renderEnd = std::numeric_limits<std::size_t>::max()) {
    if (path.size() < 2 || renderBegin + 1 >= path.size())
        return;

    const std::size_t fadeStart = renderBegin + 10;
    const std::size_t fadeEnd = renderBegin + 20;

    for (std::size_t i = renderBegin, next = renderBegin + 1;
         i + 1 < path.size() && i < renderEnd; i = next, next = i + 1) {
        const auto start = path[i].pos;
        auto end = path[next].pos;
        const baritone::BlockPos direction{end.x - start.x, end.y - start.y, end.z - start.z};

        while (next + 1 < path.size() && next + 1 < renderEnd && (!fade || next + 1 < fadeStart)) {
            const auto candidate = path[next + 1].pos;
            if (candidate.x - end.x != direction.x || candidate.y - end.y != direction.y || candidate.z - end.z != direction.z)
                break;
            end = candidate;
            ++next;
        }

        float alpha = color.a;
        if (fade) {
            if (i > fadeEnd)
                break;
            alpha *= i <= fadeStart ? 0.4f : 0.4f * (1.f - static_cast<float>(i - fadeStart) / static_cast<float>(fadeEnd - fadeStart));
        }

        pathSegment(tessellator, origin, start, end, lineOnly);
    }
}

void renderSelectionBox(Tessellator* tessellator, const glm::vec3& origin, const baritone::BlockPos& pos) {
    constexpr float inset = 0.002f;
    const float minX = pos.x + inset;
    const float maxX = pos.x + 1.f - inset;
    const float minY = pos.y + inset;
    const float maxY = pos.y + 1.f - inset;
    const float minZ = pos.z + inset;
    const float maxZ = pos.z + 1.f - inset;
    horizontalQuad(tessellator, origin, minX, maxX, minZ, maxZ, minY);
    horizontalQuad(tessellator, origin, minX, maxX, minZ, maxZ, maxY);
    line(tessellator, origin, {minX, minY, minZ}, {minX, maxY, minZ});
    line(tessellator, origin, {maxX, minY, minZ}, {maxX, maxY, minZ});
    line(tessellator, origin, {maxX, minY, maxZ}, {maxX, maxY, maxZ});
    line(tessellator, origin, {minX, minY, maxZ}, {minX, maxY, maxZ});
}

} // namespace

namespace baritone {

void PathRenderer::render(const std::vector<PathNode>& path, const std::size_t currentIndex,
    const std::vector<PathNode>& bestPath, const std::vector<PathNode>& recentPath,
    const Goal* goal, const RenderOptions& options) {
    if ((!options.renderPath && !options.renderGoal) || MC::getLevelRenderer() == nullptr || DrawUtils::getTessellator() == nullptr)
        return;

    const auto origin = MC::getLevelRenderer()->getCameraPos();
    const auto tessellator = DrawUtils::getTessellator();
    // Exact Flow/Borion DrawUtils::drawLine3d path: position-only LineList,
    // fullscreen_cube_overlay_blend, and RGB from currentShaderColor.
    auto* material = DrawUtils::getFullscreenCubeOverlayBlend();

    if (options.renderGoal && goal != nullptr) {
        DrawUtils::setShaderColor(0.f, 1.f, 0.f, 1.f);
        tessellator->begin(mce::PrimitiveMode::LineList);
        renderGoal(tessellator, origin, goal, options.animatedGoal);
        if (tessellator->getVertices() > 0)
            MeshHelpers::renderMeshImmediately(DrawUtils::getScreenContext(), tessellator, material);
    }

    if (options.renderPath && path.size() >= 2) {
        // The vertex color carries the route palette. Keep the shader multiplier neutral;
        // a red multiplier here turns every green segment black.
        DrawUtils::setShaderColor(0.f, 1.f, 0.f, 0.92f);
        tessellator->begin(mce::PrimitiveMode::LineList);
        const std::size_t renderBegin = currentIndex > 3 ? currentIndex - 3 : 0;
        // Traversed/current route is green; future route is red.
        const std::size_t traversedEnd = std::min(path.size() - 1, currentIndex + 1);
        renderRoute(tessellator, origin, path, renderBegin, options.pathAsLine, options.fadePath,
            {0.f, 1.f, 0.f, 0.92f}, traversedEnd);
        if (tessellator->getVertices() > 0)
            MeshHelpers::renderMeshImmediately(DrawUtils::getScreenContext(), tessellator, material);

        DrawUtils::setShaderColor(1.f, 0.f, 0.f, 0.92f);
        tessellator->begin(mce::PrimitiveMode::LineList);
        renderRoute(tessellator, origin, path, std::max(renderBegin, traversedEnd), options.pathAsLine, options.fadePath,
            {1.f, 0.f, 0.f, 0.92f});
        if (tessellator->getVertices() > 0)
            MeshHelpers::renderMeshImmediately(DrawUtils::getScreenContext(), tessellator, material);
    }

    if (options.renderPath && options.renderCalculations && bestPath.size() >= 2) {
        // Baritone default best path so far: #0000FF.
        DrawUtils::setShaderColor(0.f, 0.f, 1.f, 1.f);
        tessellator->begin(mce::PrimitiveMode::LineList);
        renderRoute(tessellator, origin, bestPath, 0, options.pathAsLine, options.fadePath, {0.f, 0.f, 1.f, 1.f});
        MeshHelpers::renderMeshImmediately(DrawUtils::getScreenContext(), tessellator, material);
    }

    if (options.renderPath && options.renderCalculations && !recentPath.empty()) {
        // Baritone default most recently considered path/node: #00FFFF.
        DrawUtils::setShaderColor(0.f, 1.f, 1.f, 1.f);
        tessellator->begin(mce::PrimitiveMode::LineList);
        renderRoute(tessellator, origin, recentPath, 0, options.pathAsLine, options.fadePath, {0.f, 1.f, 1.f, 1.f});
        renderSelectionBox(tessellator, origin, recentPath.back().pos);
        MeshHelpers::renderMeshImmediately(DrawUtils::getScreenContext(), tessellator, material);
    }

    if (tessellator->isTessellating())
        tessellator->clear();
    DrawUtils::setShaderColor();
}

} // namespace baritone
