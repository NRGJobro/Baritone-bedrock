#pragma once

#include <cmath>

namespace baritone::movement_input {

constexpr float componentDeadzone = 0.02f;

struct Vector {
    float x = 0.f;
    float y = 0.f;

    constexpr bool operator==(const Vector&) const = default;
};

// Bedrock accepts a two-axis stick, but its vanilla input pipeline never
// forwards a non-finite vector or one outside the unit circle. Enforce those
// invariants before the command reaches movement prediction or AuthInput.
inline Vector sanitize(Vector movement) {
    if (!std::isfinite(movement.x) || !std::isfinite(movement.y))
        return {};

    const float magnitudeSquared = movement.x * movement.x + movement.y * movement.y;
    if (magnitudeSquared > 1.f) {
        const float magnitude = std::sqrt(magnitudeSquared);
        movement.x /= magnitude;
        movement.y /= magnitude;
    }
    if (std::abs(movement.x) < componentDeadzone)
        movement.x = 0.f;
    if (std::abs(movement.y) < componentDeadzone)
        movement.y = 0.f;
    return movement;
}

struct Directions {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
};

// Keep the directional state coherent with the analog vector. The previous
// 0.35 threshold allowed movement while claiming that no direction was held.
inline Directions directions(const Vector movement) {
    return {
        .up = movement.y > 0.f,
        .down = movement.y < 0.f,
        .left = movement.x < 0.f,
        .right = movement.x > 0.f,
    };
}

} // namespace baritone::movement_input
