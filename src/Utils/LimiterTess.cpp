#include "LimiterTess.h"

#include "DrawUtils.h"
#include "../SDK/MC.h"
#include "../SDK/Render/MeshHelpers.h"

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
    tessellator->begin(mce::PrimitiveMode::LineList);

    const glm::vec3 startRelative = start - origin;
    const glm::vec3 endRelative = end - origin;
    tessellator->vertex(startRelative.x, startRelative.y, startRelative.z);
    tessellator->vertex(endRelative.x, endRelative.y, endRelative.z);

    MeshHelpers::renderMeshImmediately(screenContext3D, tessellator, material(onUi));
}

void LimiterTess::drawBox3D(const glm::vec3& lower, const glm::vec3& upper, const bool onUi) {
    if (screenContext3D == nullptr)
        return;

    auto* tessellator = screenContext3D->tessellator;
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

    MeshHelpers::renderMeshImmediately(screenContext3D, tessellator, material(onUi));
}
