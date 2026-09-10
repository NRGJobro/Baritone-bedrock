#pragma once

#include <algorithm>
#include <cmath>

namespace baritone::bedrock_physics {

// High-confidence Bedrock player constants. Positions and velocities are in
// blocks and blocks/tick; the live StateVector remains the source of truth.
constexpr float ticksPerSecond = 20.f;
constexpr float playerHalfWidth = 0.30f;
constexpr float sneakEdgeMargin = 0.025f;
constexpr float gravity = 0.08f;
constexpr float verticalDrag = 0.98f;
constexpr float jumpVelocity = 0.42f;
constexpr float horizontalAirDrag = 0.91f;
constexpr float normalSlipperiness = 0.60f;
constexpr float normalGroundDrag = horizontalAirDrag * normalSlipperiness; // 0.546
constexpr float walkGroundAcceleration = 0.098f;
constexpr float sprintGroundAcceleration = 0.1274f;
constexpr float sneakGroundAcceleration = 0.0294f;
constexpr float walkAirAcceleration = 0.0196f;
constexpr float sprintAirAcceleration = 0.02548f;
constexpr float sneakAirAcceleration = 0.00588f;
constexpr float sprintJumpBoost = 0.20f;

// Furthest centre position that retains the documented approximate sneak edge
// support margin on a full source block.
constexpr float safeTakeoffEdge = 0.5f + playerHalfWidth - sneakEdgeMargin;

inline bool shouldRetryAscent(bool grounded, float feetY, float landingY,
    bool wasAirborne, int launchTicks) {
    // Contact with a low ceiling or the near edge of the landing is not a
    // failed jump. Retry only after landing back below the intended step.
    return grounded && feetY < landingY - 0.2f && (wasAirborne || launchTicks > 6);
}

inline float waterDropAxisInput(float error, float velocity, bool grounded) {
    if (grounded)
        return std::clamp(error * 0.5f - velocity * 4.f, -0.35f, 0.35f);
    const float desiredVelocity = std::clamp(error * 0.35f, -0.12f, 0.12f);
    return std::clamp((desiredVelocity - velocity * horizontalAirDrag) /
        walkAirAcceleration, -1.f, 1.f);
}

inline bool madeWaterProgress(float horizontalDistance, float verticalRise, bool verticalSwim) {
    return horizontalDistance > 0.15f || (verticalSwim && verticalRise > 0.35f);
}

inline float groundAcceleration(const bool sprinting, const bool sneaking) {
    if (sneaking)
        return sneakGroundAcceleration;
    return sprinting ? sprintGroundAcceleration : walkGroundAcceleration;
}

inline float nextGroundVelocity(const float alongVelocity, const float forwardInput,
    const bool sprinting, const bool sneaking, const float slipperiness = normalSlipperiness) {
    const float drag = horizontalAirDrag * slipperiness;
    const float accelerationScale = std::pow(normalSlipperiness / slipperiness, 3.f);
    return alongVelocity * drag +
        std::clamp(forwardInput, -1.f, 1.f) * groundAcceleration(sprinting, sneaking) * accelerationScale;
}

inline float projectedGroundDisplacement(float alongVelocity, const float forwardInput,
    const bool sprinting, const bool sneaking, const int ticks,
    const float slipperiness = normalSlipperiness) {
    float displacement = 0.f;
    for (int tick = 0; tick < ticks; ++tick) {
        alongVelocity = nextGroundVelocity(alongVelocity, forwardInput, sprinting, sneaking, slipperiness);
        displacement += alongVelocity;
    }
    return displacement;
}

inline int ticksUntilHeight(float y, float verticalVelocity, const float targetY, const int maximumTicks = 40) {
    int ticks = 0;
    while (ticks < maximumTicks && y > targetY + 0.05f) {
        verticalVelocity = (verticalVelocity - gravity) * verticalDrag;
        y += verticalVelocity;
        ++ticks;
    }
    return ticks;
}

inline int ticksUntilLandingPlane(float y, float verticalVelocity, const float targetY,
    const int maximumTicks = 40) {
    bool descending = verticalVelocity <= 0.f;
    for (int tick = 1; tick <= maximumTicks; ++tick) {
        verticalVelocity = (verticalVelocity - gravity) * verticalDrag;
        y += verticalVelocity;
        descending = descending || verticalVelocity <= 0.f;
        if (descending && y <= targetY + 0.05f)
            return tick;
    }
    return maximumTicks;
}

inline float projectedAirDisplacement(float alongVelocity, const float forwardInput,
    const bool sprinting, const bool sneaking, const int ticks) {
    const float acceleration = sneaking ? sneakAirAcceleration :
        (sprinting ? sprintAirAcceleration : walkAirAcceleration);
    float displacement = 0.f;
    for (int tick = 0; tick < ticks; ++tick) {
        alongVelocity = alongVelocity * horizontalAirDrag +
            std::clamp(forwardInput, -1.f, 1.f) * acceleration;
        displacement += alongVelocity;
    }
    return displacement;
}

} // namespace baritone::bedrock_physics
