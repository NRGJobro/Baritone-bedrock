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

// A 0.6-wide player whose centre is within this per-axis band still has a
// useful support margin on every side of a full block. Requiring the exact
// 0.06-radius centre caused Bedrock's digital direction flags to repeatedly
// overshoot and reverse while preparing consecutive downward mines.
constexpr float shaftCenteredAxisTolerance = 0.18f;

inline bool shaftFootprintCentered(const float offsetX, const float offsetZ) {
    return std::isfinite(offsetX) && std::isfinite(offsetZ) &&
        std::abs(offsetX) <= shaftCenteredAxisTolerance &&
        std::abs(offsetZ) <= shaftCenteredAxisTolerance;
}

// Furthest centre position that retains the documented approximate sneak edge
// support margin on a full source block.
constexpr float safeTakeoffEdge = 0.5f + playerHalfWidth - sneakEdgeMargin;

inline bool parkourSprintReady(int distance, bool ascending, bool sprintEnabled,
    int sprintTicks, float measuredSpeed) {
    if (distance < 3 && !ascending)
        return true;
    const float minimumSpeed = distance >= 4 ? 0.20f : 0.17f;
    return sprintEnabled && sprintTicks >= 2 && measuredSpeed >= minimumSpeed;
}

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

inline float groundInputForTargetVelocity(const float alongVelocity,
    const float targetVelocity, const bool sprinting = false,
    const bool sneaking = false, const float slipperiness = normalSlipperiness) {
    const float drag = horizontalAirDrag * slipperiness;
    const float accelerationScale = std::pow(normalSlipperiness / slipperiness, 3.f);
    const float acceleration = groundAcceleration(sprinting, sneaking) * accelerationScale;
    if (!std::isfinite(alongVelocity) || !std::isfinite(targetVelocity) ||
        acceleration <= 0.0001f)
        return 0.f;
    return std::clamp((targetVelocity - alongVelocity * drag) / acceleration, -1.f, 1.f);
}

constexpr float cautiousDropSneakReleaseProgress = 0.78f;

inline float cautiousDropGroundInput(const float progress, const float alongVelocity) {
    if (!std::isfinite(progress) || !std::isfinite(alongVelocity) ||
        progress >= cautiousDropSneakReleaseProgress)
        return 0.f;
    // Native direction flags are digital. Creep with short sneak-speed pulses,
    // then release sneak and forward together before the support boundary.
    return alongVelocity <= 0.060f ? 1.f : 0.f;
}

inline float shaftCenterInput(const float centerDistance, const float velocityTowardCenter) {
    // A downward mine must never steer through/past the centre to cancel
    // momentum. Sneak movement and ground friction do the braking; this
    // command can only point toward the centre of the supporting block.
    if (!std::isfinite(centerDistance) || !std::isfinite(velocityTowardCenter) ||
        centerDistance <= shaftCenteredAxisTolerance)
        return 0.f;
    const float targetVelocity = std::clamp(centerDistance * 0.18f, 0.018f, 0.055f);
    return std::clamp(groundInputForTargetVelocity(
        velocityTowardCenter, targetVelocity, false, true), 0.f, 0.32f);
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

inline bool needsCautiousDropApproach(const int verticalDrop, const bool waterDrop,
    const float progress, const float alongVelocity, const float segmentLength,
    const int landingTicks) {
    if (verticalDrop < 2 || waterDrop || !std::isfinite(progress) ||
        !std::isfinite(alongVelocity) || segmentLength <= 0.001f)
        return false;
    const float projectedProgress = progress + projectedAirDisplacement(
        std::max(alongVelocity, 0.f), 1.f, false, false, landingTicks) /
        segmentLength;
    return projectedProgress > 1.15f;
}

inline float fallLandingInput(const float progress, const float alongVelocity,
    const float segmentLength, const int landingTicks,
    const float targetProgress = 0.90f) {
    if (!std::isfinite(progress) || !std::isfinite(alongVelocity) ||
        !std::isfinite(segmentLength) || segmentLength <= 0.001f || landingTicks <= 0)
        return 0.f;

    const float coastDisplacement = projectedAirDisplacement(
        std::max(alongVelocity, 0.f), 0.f, false, false, landingTicks);
    const float forwardDisplacement = projectedAirDisplacement(
        std::max(alongVelocity, 0.f), 1.f, false, false, landingTicks);
    const float inputEffect = forwardDisplacement - coastDisplacement;
    if (inputEffect <= 0.0001f)
        return 0.f;

    const float desiredDisplacement = (targetProgress - progress) * segmentLength;
    // A normal player releases forward input to shed momentum during a drop;
    // they do not tap backward for one frame in midair. Ground-speed control
    // makes reverse air input unnecessary for ordinary descents.
    return std::clamp((desiredDisplacement - coastDisplacement) / inputEffect, 0.f, 1.f);
}

inline float dropAirInput(const int verticalDrop, const float progress,
    const float alongVelocity, const float segmentLength, const int landingTicks,
    const float targetProgress = 0.90f) {
    // Once a longer drop starts, release every forward direction flag. The
    // controlled edge-entry speed supplies enough momentum and coasting avoids
    // the full-key acceleration caused by Bedrock's digital input state.
    if (verticalDrop >= 2)
        return 0.f;
    return fallLandingInput(progress, alongVelocity, segmentLength,
        landingTicks, targetProgress);
}

} // namespace baritone::bedrock_physics
