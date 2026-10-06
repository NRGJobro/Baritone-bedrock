#pragma once

#include <array>
#include <utility>
#include <vector>

class ScreenContext;
namespace mce { class Color; }

// Limiter's 3D tessellation API, backed exclusively by the current SDK.
class LimiterTess {
public:
    static void setTessellator3D(ScreenContext* context);
    static void setColor(const mce::Color& color);
    static void drawLine3D(const glm::vec3& start, const glm::vec3& end, bool onUi = true);
    static void drawLineList3D(const std::vector<std::pair<glm::vec3, glm::vec3>>& lines,
        bool onUi = true);
    static void drawFilledQuad3D(const std::array<glm::vec3, 4>& corners, bool onUi = true);
    static void drawFilledQuads3D(const std::vector<std::array<glm::vec3, 4>>& quads,
        bool onUi = true);
    static void drawFilledBox3D(const glm::vec3& lower, const glm::vec3& upper,
        bool onUi = true);
    static void drawBox3D(const glm::vec3& lower, const glm::vec3& upper, bool onUi = true);
};
