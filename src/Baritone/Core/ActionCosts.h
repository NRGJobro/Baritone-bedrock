#pragma once

#include "../Bedrock/BedrockPhysics.h"

#include <algorithm>

namespace baritone::action_costs {

// Baritone's Java ActionCosts expressed against Bedrock's measured steady
// speeds. These are approximate movement ticks, not direct velocity writes.
constexpr double walkOneBlock = 1.0 / (bedrock_physics::walkGroundAcceleration /
    (1.0 - bedrock_physics::normalGroundDrag));
constexpr double sprintOneBlock = 1.0 / (bedrock_physics::sprintGroundAcceleration /
    (1.0 - bedrock_physics::normalGroundDrag));
constexpr double walkInWater = 20.0 / 2.2;
constexpr double sprintMultiplier = sprintOneBlock / walkOneBlock;
constexpr double walkOffBlock = walkOneBlock * 0.8;
constexpr double centerAfterFall = walkOneBlock - walkOffBlock;
constexpr double costInf = 1000000.0;

inline double fallTicks(const int blocks) {
    if (blocks <= 0)
        return 0.0;
    float y = static_cast<float>(blocks);
    float velocity = 0.f;
    int ticks = 0;
    while (y > 0.05f && ticks < 256) {
        velocity = (velocity - bedrock_physics::gravity) * bedrock_physics::verticalDrag;
        y += velocity;
        ++ticks;
    }
    return static_cast<double>(ticks);
}

inline double jumpOneBlock() {
    return std::max(walkOneBlock, 7.0);
}

inline double parkourJump(const int distance, const bool sprint) {
    if (distance <= 2)
        return walkOneBlock * 2.0;
    if (distance == 3)
        return walkOneBlock * 3.0;
    return (sprint ? sprintOneBlock : walkOneBlock) * static_cast<double>(distance);
}

} // namespace baritone::action_costs
