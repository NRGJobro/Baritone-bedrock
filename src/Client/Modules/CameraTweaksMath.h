#pragma once

#include <algorithm>
#include <cmath>

namespace CameraTweaksMath {

    inline bool acceptsScroll(const int perspective, const bool gameplay, const bool guiOpen) {
        return (perspective == 1 || perspective == 2) && gameplay && !guiOpen;
    }

    inline float scrollDistance(float distance, float step, const bool up) {
        if (!std::isfinite(distance))
            distance = 4.f;
        if (!std::isfinite(step))
            step = 0.5f;
        return std::clamp(distance + (up ? -1.f : 1.f) * std::clamp(step, 0.1f, 4.f), 0.5f, 32.f);
    }

    inline float approach(float current, float target, const float deltaTime) {
        if (!std::isfinite(target))
            target = 4.f;
        target = std::clamp(target, 0.5f, 32.f);
        if (!std::isfinite(current))
            return target;
        return current + (target - current) * (1.f - std::exp(-12.f * std::clamp(deltaTime, 0.f, 0.1f)));
    }

}  // namespace CameraTweaksMath
