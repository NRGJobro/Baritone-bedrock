#include "LimiterTess.h"

#include "DrawUtils.h"
#include "../SDK/MC.h"
#include "../SDK/Client/MCE/Color.h"
#include "../SDK/Render/Level/LevelRenderer.h"
#include "../SDK/Render/MeshHelpers.h"
#include "../SDK/Render/ScreenContext.h"
#include "../SDK/Render/Tessellator.h"

#include <algorithm>
#include <climits>

namespace {
ScreenContext* screenContext3D = nullptr;
glm::vec3 origin{};

mce::MaterialPtr* material(const bool onUi) {
    return onUi ? DrawUtils::getUIFillColor() : DrawUtils::getSelectionOverlay();
}
}

void LimiterTess::setTessellator3D(ScreenContext* context) {
    screenContext3D = context;
    if (context != nullptr && MC::getLevelRenderer() != nullptr)
        origin = MC::getLevelRenderer()->getCameraPos();
}

void LimiterTess::setColor(const mce::Color& color) {
    DrawUtils::setShaderColor(color.r, color.g, color.b, color.a);
}

void LimiterTess::drawLine3D(const glm::vec3& start, const glm::vec3& end, const bool onUi) {
    if (screenContext3D == nullptr)
        return;

    auto* tessellator = screenContext3D->tessellator;
    if (tessellator == nullptr)
        return;
    tessellator->begin(mce::PrimitiveMode::LineList);

    const glm::vec3 startRelative = start - origin;
    const glm::vec3 endRelative = end - origin;
    tessellator->vertex(startRelative.x, startRelative.y, startRelative.z);
    tessellator->vertex(endRelative.x, endRelative.y, endRelative.z);

    MeshHelpers::renderMeshImmediately(screenContext3D, tessellator, drawMaterial);
}

void LimiterTess::drawLineList3D(
    const std::vector<std::pair<glm::vec3, glm::vec3>>& lines, const bool onUi) {
    if (screenContext3D == nullptr || lines.empty())
        return;

    auto* tessellator = screenContext3D->tessellator;
    if (tessellator == nullptr)
        return;
    tessellator->begin(mce::PrimitiveMode::LineList,
        static_cast<int>(std::min<size_t>(lines.size() * 2, static_cast<size_t>(INT_MAX))));
    for (const auto& [start, end] : lines) {
        const glm::vec3 startRelative = start - origin;
        const glm::vec3 endRelative = end - origin;
        tessellator->vertex(startRelative.x, startRelative.y, startRelative.z);
        tessellator->vertex(endRelative.x, endRelative.y, endRelative.z);
    }

    MeshHelpers::renderMeshImmediately(screenContext3D, tessellator, drawMaterial);
}

void LimiterTess::drawFilledQuad3D(const std::array<glm::vec3, 4>& corners,
    const bool onUi) {
    drawFilledQuads3D({corners}, onUi);
}

void LimiterTess::drawFilledQuads3D(
    const std::vector<std::array<glm::vec3, 4>>& quads, const bool onUi) {
    if (screenContext3D == nullptr || quads.empty())
        return;

    auto* tessellator = screenContext3D->tessellator;
    if (tessellator == nullptr)
        return;
    tessellator->begin(mce::PrimitiveMode::QuadList,
        static_cast<int>(std::min<size_t>(quads.size() * 4, static_cast<size_t>(INT_MAX))));
    for (const auto& corners : quads) {
        for (const auto& corner : corners) {
            const glm::vec3 relative = corner - origin;
            tessellator->vertex(relative.x, relative.y, relative.z);
        }
    }

    MeshHelpers::renderMeshImmediately(screenContext3D, tessellator, drawMaterial);
}

void LimiterTess::drawFilledBox3D(const glm::vec3& lower, const glm::vec3& upper,
    const bool onUi) {
    const glm::vec3& l = lower;
    const glm::vec3& u = upper;
    drawFilledQuads3D({
        {{{l.x, l.y, l.z}, {l.x, u.y, l.z}, {l.x, u.y, u.z}, {l.x, l.y, u.z}}},
        {{{u.x, l.y, l.z}, {u.x, l.y, u.z}, {u.x, u.y, u.z}, {u.x, u.y, l.z}}},
        {{{l.x, l.y, l.z}, {l.x, l.y, u.z}, {u.x, l.y, u.z}, {u.x, l.y, l.z}}},
        {{{l.x, u.y, l.z}, {u.x, u.y, l.z}, {u.x, u.y, u.z}, {l.x, u.y, u.z}}},
        {{{l.x, l.y, l.z}, {u.x, l.y, l.z}, {u.x, u.y, l.z}, {l.x, u.y, l.z}}},
        {{{l.x, l.y, u.z}, {l.x, u.y, u.z}, {u.x, u.y, u.z}, {u.x, l.y, u.z}}}
    }, onUi);
}

void LimiterTess::drawBox3D(const glm::vec3& lower, const glm::vec3& upper, const bool onUi) {
    if (screenContext3D == nullptr)
        return;

    auto* tessellator = screenContext3D->tessellator;
    if (tessellator == nullptr)
        return;
    const glm::vec3 difference = upper - lower;
    const glm::vec3 newLower = lower - origin;
    const glm::vec3 vertices[8]{
        {newLower.x, newLower.y, newLower.z},
        {newLower.x + difference.x, newLower.y, newLower.z},
        {newLower.x, newLower.y, newLower.z + difference.z},
        {newLower.x + difference.x, newLower.y, newLower.z + difference.z},
        {newLower.x, newLower.y + difference.y, newLower.z},
        {newLower.x + difference.x, newLower.y + difference.y, newLower.z},
        {newLower.x, newLower.y + difference.y, newLower.z + difference.z},
        {newLower.x + difference.x, newLower.y + difference.y, newLower.z + difference.z}
    };

    constexpr int edges[24]{
        4, 5, 5, 7, 7, 6, 6, 4,
        0, 1, 1, 3, 3, 2, 2, 0,
        0, 4, 1, 5, 2, 6, 3, 7
    };
    tessellator->begin(mce::PrimitiveMode::LineList);
    for (const int vertexIndex : edges)
        tessellator->vertex(vertices[vertexIndex]);

    MeshHelpers::renderMeshImmediately(screenContext3D, tessellator, drawMaterial);
}
