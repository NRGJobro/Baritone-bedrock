#pragma once

#include "../Core/BlockPos.h"

#include <glm/glm.hpp>
#include <optional>
#include <string>

class LocalPlayer;
class PlayerAuthInputPacket;

namespace baritone {

class ElytraProcess {
    static bool sGliding;
    static float sYaw;
    static float sPitch;

    BlockPos target{};
    bool active = false;
    bool rotationPrimed = false;
    int rocketCooldown = 0;
    int takeoffTicks = 0;
    int rocketInterval = 30;
    int savedHotbarSlot = -1;
    std::optional<std::string> pendingMessage;

    static float wrapDegrees(float value);
    static bool terrainAhead(const glm::vec3& position, float yaw, float pitch);
    static int findRocketSlot(LocalPlayer* player);
    bool feedRocket(LocalPlayer* player);
    void restoreHotbar(LocalPlayer* player);

public:
    void start(const BlockPos& target);
    void cancel(LocalPlayer* player = nullptr);
    void suspend(LocalPlayer* player = nullptr);
    void tick(LocalPlayer* player);
    void render() const;

    static void rewriteAuthInput(PlayerAuthInputPacket& packet);

    [[nodiscard]] bool isActive() const { return active; }
    [[nodiscard]] int& getRocketInterval() { return rocketInterval; }
    [[nodiscard]] std::string getStatusLine() const;
    std::optional<std::string> takeMessage();
};

} // namespace baritone
