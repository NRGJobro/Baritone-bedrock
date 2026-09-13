#pragma once

#include "../../SDK/World/Level/HitResult/FacingID.h"

#include <glm/glm.hpp>

class LocalPlayer;
class PlayerAuthInputPacket;

namespace baritone::bedrock_block_breaking {

// Advances one tick through Bedrock's native held-left-click callback.
void tick(LocalPlayer* player, const glm::ivec3& target, FacingID face,
    const glm::vec3& playerPosition);
void stop(LocalPlayer* player, const glm::ivec3& target);
// Replaces competing native crosshair actions with one server-facing mining
// target and rotation immediately before PlayerAuthInput is serialized.
void rewritePlayerAuthInput(PlayerAuthInputPacket& packet);
// Sends a completed destroy transaction after the silently rotated auth input
// for that tick has already gone to the server.
void flushCommit();

} // namespace baritone::bedrock_block_breaking
