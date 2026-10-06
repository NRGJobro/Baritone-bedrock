#include "ElytraProcess.h"

#include "BedrockWorld.h"

#include "../../SDK/MC.h"
#include "../../Utils/LimiterTess.h"
#include "../../SDK/Network/Packet/Packets/PlayerAuthInputPacket.h"
#include "../../SDK/World/Actor/Components/StateVectorComponent.h"
#include "../../SDK/World/Actor/LocalPlayer.h"
#include "../../SDK/World/Item/Item.h"
#include "../../SDK/World/Item/ItemStack.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace baritone {

bool ElytraProcess::sGliding = false;
float ElytraProcess::sYaw = 0.f;
float ElytraProcess::sPitch = 0.f;

constexpr int elytraStartGlidingFlag = 32;
constexpr int elytraStopGlidingFlag = 33;
constexpr float elytraArrivalHorizontal = 8.f;
constexpr float elytraMaxPitch = 35.f;
constexpr float elytraMinCruiseSpeed = 0.35f;
constexpr float elytraMaxTurnPerTick = 14.f;
constexpr int elytraHeadingRayLength = 24;

float ElytraProcess::wrapDegrees(float value) {
    while (value > 180.f) value -= 360.f;
    while (value < -180.f) value += 360.f;
    return value;
}

void ElytraProcess::start(const BlockPos& goal) {
    target = goal;
    active = true;
    rocketCooldown = 0;
    takeoffTicks = 0;
    rotationPrimed = false;
}

void ElytraProcess::cancel(LocalPlayer* player) {
    restoreHotbar(player != nullptr ? player : MC::getLocalPlayer());
    active = false;
    sGliding = false;
    takeoffTicks = 0;
    rocketCooldown = 0;
    rotationPrimed = false;
}

void ElytraProcess::resetForWorldChange() {
    active = false;
    sGliding = false;
    sYaw = 0.f;
    sPitch = 0.f;
    target = {};
    rotationPrimed = false;
    rocketCooldown = 0;
    takeoffTicks = 0;
    savedHotbarSlot = -1;
    pendingMessage.reset();
}

void ElytraProcess::suspend(LocalPlayer* player) {
    restoreHotbar(player != nullptr ? player : MC::getLocalPlayer());
    sGliding = false;
    rotationPrimed = false;
}

bool ElytraProcess::terrainAhead(const glm::vec3& position, float yaw, float pitch) {
    auto* region = MC::getRegion();
    if (region == nullptr)
        return false;

    const float yawRadians = yaw * std::numbers::pi_v<float> / 180.f;
    const float pitchRadians = pitch * std::numbers::pi_v<float> / 180.f;
    const float horizontal = std::cos(pitchRadians);
    const glm::vec3 forward{
        -std::sin(yawRadians) * horizontal,
        -std::sin(pitchRadians),
        std::cos(yawRadians) * horizontal};

    const BedrockWorld world{region};
    for (int step = 2; step <= elytraHeadingRayLength; ++step) {
        const glm::vec3 sample = position + forward * static_cast<float>(step);
        const BlockPos probe{
            static_cast<int>(std::floor(sample.x)),
            static_cast<int>(std::floor(sample.y)),
            static_cast<int>(std::floor(sample.z))};
        const BlockState state = world.getBlock(probe);
        if (state.loaded && state.solid)
            return true;
    }
    return false;
}

int ElytraProcess::findRocketSlot(LocalPlayer* player) {
    auto* supplies = player->getSupplies();
    if (supplies == nullptr)
        return -1;
    auto* inventory = supplies->getInventory();
    if (inventory == nullptr)
        return -1;

    for (int slot = 0; slot < 9; ++slot) {
        ItemStack* stack = inventory->getItem(slot);
        if (stack == nullptr || !stack->isValid())
            continue;
        const Item* item = stack->getItem();
        if (item != nullptr && item->getRawName() == "firework_rocket")
            return slot;
    }
    return -1;
}

bool ElytraProcess::feedRocket(LocalPlayer* player) {
    auto* supplies = player->getSupplies();
    auto* gameMode = player->getGameMode();
    if (supplies == nullptr || gameMode == nullptr)
        return false;

    const int rocketSlot = findRocketSlot(player);
    if (rocketSlot < 0)
        return false;

    if (supplies->getSelectedHotbarSlot() != rocketSlot) {
        if (savedHotbarSlot < 0)
            savedHotbarSlot = supplies->getSelectedHotbarSlot();
        supplies->setSelectedHotbarSlot(rocketSlot);
        return true;
    }

    auto* inventory = supplies->getInventory();
    if (inventory == nullptr)
        return false;
    ItemStack* stack = inventory->getItem(rocketSlot);
    if (stack == nullptr || !stack->isValid())
        return false;

    gameMode->useItem(*stack);
    rocketCooldown = rocketInterval;
    return true;
}

void ElytraProcess::restoreHotbar(LocalPlayer* player) {
    if (savedHotbarSlot < 0 || player == nullptr)
        return;
    if (auto* supplies = player->getSupplies())
        supplies->setSelectedHotbarSlot(savedHotbarSlot);
    savedHotbarSlot = -1;
}

void ElytraProcess::tick(LocalPlayer* player) {
    if (!active) {
        sGliding = false;
        return;
    }
    if (player == nullptr)
        return;

    const auto* state = player->tryGet<StateVectorComponent>();
    if (state == nullptr)
        return;

    const glm::vec3 position = state->pos;
    const glm::vec3 goal{
        static_cast<float>(target.x) + 0.5f,
        static_cast<float>(target.y),
        static_cast<float>(target.z) + 0.5f};
    const glm::vec3 delta = goal - position;
    const float horizontal = std::sqrt(delta.x * delta.x + delta.z * delta.z);

    if (horizontal < elytraArrivalHorizontal) {
        pendingMessage = "Arrived at flight target.";
        cancel(player);
        return;
    }

    if (player->isOnGround()) {
        sGliding = false;
        if (takeoffTicks == 0) {
            player->jumpFromGround();
            takeoffTicks = 4;
        } else {
            --takeoffTicks;
        }
        rocketCooldown = 0;
        return;
    }
    takeoffTicks = 0;
    sGliding = true;

    const float desiredYaw = std::atan2(-delta.x, delta.z) * 180.f / std::numbers::pi_v<float>;
    float desiredPitch = -std::atan2(delta.y, std::max(horizontal, 1.f)) * 180.f / std::numbers::pi_v<float>;
    desiredPitch = std::clamp(desiredPitch, -elytraMaxPitch, elytraMaxPitch);
    if (terrainAhead(position, desiredYaw, desiredPitch))
        desiredPitch = -elytraMaxPitch;

    if (!rotationPrimed) {
        const glm::vec2 current = player->getRotation();
        sYaw = current.y;
        sPitch = current.x;
        rotationPrimed = true;
    }

    const float yawDelta = std::clamp(wrapDegrees(desiredYaw - sYaw),
        -elytraMaxTurnPerTick, elytraMaxTurnPerTick);
    const float pitchDelta = std::clamp(desiredPitch - sPitch,
        -elytraMaxTurnPerTick, elytraMaxTurnPerTick);
    sYaw = wrapDegrees(sYaw + yawDelta);
    sPitch = std::clamp(sPitch + pitchDelta, -90.f, 90.f);

    player->setRotation(glm::vec2{sPitch, sYaw});

    const float speed = std::sqrt(
        state->velocity.x * state->velocity.x + state->velocity.z * state->velocity.z);

    if (rocketCooldown > 0 && speed >= elytraMinCruiseSpeed) {
        --rocketCooldown;
        return;
    }

    if (!feedRocket(player)) {
        pendingMessage = "Out of fireworks.";
        cancel(player);
    }
}

void ElytraProcess::rewriteAuthInput(PlayerAuthInputPacket& packet) {
    if (!sGliding)
        return;
    packet.inputFlags.set(elytraStartGlidingFlag, true);
    packet.inputFlags.set(elytraStopGlidingFlag, false);
    packet.pitch = sPitch;
    packet.yaw = sYaw;
    packet.bodyYaw = sYaw;
}

void ElytraProcess::render() const {
    if (!active || MC::getLevelRenderer() == nullptr)
        return;

    auto* player = MC::getLocalPlayer();
    if (player == nullptr)
        return;

    const auto* state = player->tryGet<StateVectorComponent>();
    if (state == nullptr)
        return;

    const glm::vec3 from{state->pos.x, state->pos.y, state->pos.z};
    const glm::vec3 to{
        static_cast<float>(target.x) + 0.5f,
        static_cast<float>(target.y) + 0.5f,
        static_cast<float>(target.z) + 0.5f};

    LimiterTess::setColor({125, 245, 255, 235});
    LimiterTess::drawLine3D(from, to, true);

    const glm::vec3 lower{
        static_cast<float>(target.x), static_cast<float>(target.y), static_cast<float>(target.z)};
    const glm::vec3 upper{lower.x + 1.f, lower.y + 1.f, lower.z + 1.f};
    LimiterTess::setColor({35, 255, 80, 235});
    LimiterTess::drawBox3D(lower, upper, true);

    const float yawRadians = sYaw * std::numbers::pi_v<float> / 180.f;
    const float pitchRadians = sPitch * std::numbers::pi_v<float> / 180.f;
    const float horizontal = std::cos(pitchRadians);
    const glm::vec3 heading{
        -std::sin(yawRadians) * horizontal,
        -std::sin(pitchRadians),
        std::cos(yawRadians) * horizontal};

    LimiterTess::setColor({255, 190, 60, 200});
    LimiterTess::drawLine3D(from, from + heading * static_cast<float>(elytraHeadingRayLength), true);
}

std::string ElytraProcess::getStatusLine() const {
    if (!active)
        return "Elytra: idle";
    return "Elytra: flying to " + std::to_string(target.x) + " " +
        std::to_string(target.y) + " " + std::to_string(target.z);
}

std::optional<std::string> ElytraProcess::takeMessage() {
    if (!pendingMessage.has_value())
        return std::nullopt;
    auto message = std::move(*pendingMessage);
    pendingMessage.reset();
    return message;
}

} // namespace baritone
