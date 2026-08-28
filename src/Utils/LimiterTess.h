#pragma once

#include "../SDK/Client/MCE/Color.h"

class ScreenContext;

// Limiter's 3D tessellation API, backed exclusively by the current SDK.
class LimiterTess {
public:
    static void setTessellator3D(ScreenContext* context);
    static void setColor(const mce::Color& color);
    static void drawLine3D(const glm::vec3& start, const glm::vec3& end, bool onUi = true);
    static void drawBox3D(const glm::vec3& lower, const glm::vec3& upper, bool onUi = true);
};
