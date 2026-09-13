#include "BedrockBlockBreaking.h"

#include "../../SDK/MC.h"
#include "../../SDK/Network/Packet/Packets/PlayerAuthInputPacket.h"
#include "../../SDK/World/Actor/GameMode.h"
#include "../../SDK/World/Actor/LocalPlayer.h"
#include "../../SDK/World/BlockSource.h"
#include "../../Utils/Logger.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>
#include <string_view>

namespace baritone::bedrock_block_breaking {
namespace {

struct PendingBreak {
    LocalPlayer* player = nullptr;
    glm::ivec3 target{};
    FacingID face = FacingID::Unknown;
    float pitch = 0.f;
    float yaw = 0.f;
    bool startPending = true;
    bool commitPending = false;
    bool commitSent = false;
    bool nativeCommitArmed = false;
    int crackTicks = 0;
    int requiredTicks = 12;
    int aimPacketsRemaining = 3;
    std::bitset<65> lastInputFlags{};
};

std::optional<PendingBreak> pendingBreak;
std::optional<glm::ivec3> pendingAbort;

glm::vec3 faceCenter(const glm::ivec3& target, const FacingID face) {
    glm::vec3 point{target.x + 0.5f, target.y + 0.5f, target.z + 0.5f};
    switch (face) {
    case FacingID::Down: point.y = static_cast<float>(target.y); break;
    case FacingID::Up: point.y = static_cast<float>(target.y + 1); break;
    case FacingID::North: point.z = static_cast<float>(target.z); break;
    case FacingID::South: point.z = static_cast<float>(target.z + 1); break;
    case FacingID::West: point.x = static_cast<float>(target.x); break;
    case FacingID::East: point.x = static_cast<float>(target.x + 1); break;
    default: break;
    }
    return point;
}

int calculateRequiredTicks(LocalPlayer* player, const glm::ivec3& target) {
    auto* region = MC::getRegion();
    auto* block = region == nullptr ? nullptr : region->getBlock(target);
    const std::string_view name = block == nullptr || block->getBlockLegacy() == nullptr
        ? std::string_view{} : block->getBlockLegacy()->getName();

    // Lifeboat's custom tools return non-vanilla destroy speeds (including
    // zero), so ItemStack::getDestroySpeed cannot be used as a timer there.
    // The enhanced manual capture completed one ore at 19 packets, but
    // Lifeboat uses slightly different timings across ore tiers. Keep a
    // conservative margin so we never complete on the first packet the
    // fastest observed block happened to accept. 24 packets still caused
    // Lifeboat to roll back some ore tiers before accepting the retry.
    if (name.find("obsidian") != std::string_view::npos)
        return 200;
    if (name.find("ancient_debris") != std::string_view::npos)
        return 120;
    (void)player;
    return 32;
}

} // namespace

void tick(LocalPlayer* player, const glm::ivec3& target, const FacingID face,
    const glm::vec3& playerPosition) {
    if (player == nullptr || player->getGameMode() == nullptr)
        return;

    // Aim from the player's eye position at the center of the exact face sent
    // in the block action. Aiming at the block center can put the packet ray
    // through a neighboring block, which dedicated servers reject.
    const glm::vec3 aimPoint = faceCenter(target, face);
    const glm::vec3 ray = aimPoint - playerPosition;
    const float horizontal = std::sqrt(ray.x * ray.x + ray.z * ray.z);
    const bool changedTarget = !pendingBreak || pendingBreak->target != target;
    const float pitch = std::clamp(
        -std::atan2(ray.y, horizontal) * 180.f / std::numbers::pi_v<float>,
        -89.9f, 89.9f);
    // Yaw is undefined for a nearly vertical ray. Reusing it prevents the
    // silent packet rotation from flipping by 180 degrees between ticks.
    const float yaw = horizontal > 0.001f
        ? std::atan2(-ray.x, ray.z) * 180.f / std::numbers::pi_v<float>
        : (!changedTarget ? pendingBreak->yaw : player->getRotation().y);

    if (changedTarget) {
        if (pendingBreak) {
            pendingAbort = pendingBreak->target;
            if (pendingBreak->player != nullptr)
                hat::member_at<bool>(pendingBreak->player, 0xAFA) = false;
        }
        pendingBreak = PendingBreak{};
        pendingBreak->player = player;
        pendingBreak->target = target;
        pendingBreak->face = face;
        pendingBreak->pitch = pitch;
        pendingBreak->yaw = yaw;
        pendingBreak->requiredTicks = calculateRequiredTicks(player, target);
        logF("[BlockBreakState] begin pos=({}, {}, {}) ticks={}",
            target.x, target.y, target.z, pendingBreak->requiredTicks);
    } else {
        pendingBreak->player = player;
        pendingBreak->face = face;
        pendingBreak->pitch = pitch;
        pendingBreak->yaw = yaw;
    }

    // Keep the local camera/actor rotation identical to the values written to
    // PlayerAuthInput. This also makes Bedrock's native transaction ray agree
    // with the face-center ray used by the server.
    player->setRotation({pitch, yaw});
    if (auto* head = player->tryGet<ActorHeadRotationComponent>())
        head->rotation = {yaw, yaw};
    if (auto* body = player->tryGet<MobBodyRotationComponent>()) {
        body->bodyRotation = yaw;
        body->previousBodyRotation = yaw;
    }

    // Advance the same native destroy context as held-mouse mining. Lifeboat's
    // item transaction is only accepted when this context has been updated on
    // every tick leading up to completion.
    auto* gameMode = player->getGameMode();
    if (pendingBreak->commitPending && !pendingBreak->commitSent &&
        !pendingBreak->nativeCommitArmed) {
        auto* hitWrapper = player->getLevel() == nullptr
            ? nullptr : player->getLevel()->getHitResultWrapper();
        std::optional<HitResult> savedHit;
        if (hitWrapper != nullptr) {
            savedHit = hitWrapper->hitResult;
            auto& hit = hitWrapper->hitResult;
            hit.startPos = player->getPosition();
            hit.type = HitResultType::Tile;
            hit.facing = pendingBreak->face;
            hit.blockPos = pendingBreak->target;
            hit.pos = faceCenter(pendingBreak->target, pendingBreak->face);
            const glm::vec3 hitRay = hit.pos - hit.startPos;
            const float hitLength = glm::length(hitRay);
            hit.rayDir = hitLength > 0.0001f ? hitRay / hitLength : glm::vec3{0.f, -1.f, 0.f};
        }
        // Complete through the same simulation call used by held-mouse mining.
        // Calling destroyBlock directly creates a local prediction but skips
        // the active destroy context used to build/validate durability data.
        hat::member_at<float>(gameMode, 0x24) = 1.f;
        bool destroyed = false;
        gameMode->continueDestroyBlock(pendingBreak->target, pendingBreak->face,
            playerPosition, destroyed);
        if (!destroyed) {
            auto commitTarget = pendingBreak->target;
            gameMode->destroyBlock(&commitTarget, pendingBreak->face);
        }
        if (hitWrapper != nullptr && savedHit)
            hitWrapper->hitResult = *savedHit;
        pendingBreak->commitPending = false;
        pendingBreak->nativeCommitArmed = true;
        logF("[BlockBreakState] native completion armed pos=({}, {}, {}) face={} signal={}",
            target.x, target.y, target.z, static_cast<int>(pendingBreak->face), destroyed);
        player->swing();
        return;
    }
    if (!pendingBreak->commitSent) {
        auto& isDestroying = hat::member_at<bool>(player, 0xAFA);
        isDestroying = true;
        bool destroyed = false;
        if (changedTarget) {
            gameMode->startDestroyBlock(target, face, destroyed);
        } else {
            gameMode->continueDestroyBlock(target, face, playerPosition, destroyed);
        }
        const float progress = hat::member_at<float>(gameMode, 0x24);
        if (destroyed || progress >= 0.999f) {
            pendingBreak->commitPending = false;
            pendingBreak->nativeCommitArmed = true;
            logF("[BlockBreakState] native completion reached pos=({}, {}, {}) progress={:.3f} signal={}",
                target.x, target.y, target.z, progress, destroyed);
        }
    }
    player->swing();
}

void stop(LocalPlayer* player, const glm::ivec3& target) {
    const bool completionWasSent = pendingBreak && pendingBreak->commitSent;
    if (player != nullptr) {
        hat::member_at<bool>(player, 0xAFA) = false;
        if (player->getGameMode() != nullptr)
            player->getGameMode()->stopDestroyBlock(target);
    }
    // Do not follow a completed native transaction with AbortDestroyBlock.
    // Lifeboat treats that immediate abort as cancellation and restores the
    // locally predicted block before eventually processing another attempt.
    if (completionWasSent)
        pendingAbort.reset();
    else
        pendingAbort = pendingBreak ? pendingBreak->target : std::optional<glm::ivec3>{target};
    pendingBreak.reset();
}

void rewritePlayerAuthInput(PlayerAuthInputPacket& packet) {
    if (!pendingBreak && !pendingAbort)
        return;

    auto applySilentRotation = [&packet](const PendingBreak& pending) {
        packet.pitch = pending.pitch;
        packet.yaw = pending.yaw;
        packet.bodyYaw = pending.yaw;
        packet.interactRotation = {pending.pitch, pending.yaw};
        const float pitchRadians = pending.pitch * std::numbers::pi_v<float> / 180.f;
        const float yawRadians = pending.yaw * std::numbers::pi_v<float> / 180.f;
        const float horizontal = std::cos(pitchRadians);
        packet.cameraOrientation = {
            -std::sin(yawRadians) * horizontal,
            -std::sin(pitchRadians),
            std::cos(yawRadians) * horizontal};
    };

    // GameMode::destroyBlock supplies the opaque item-interaction payload that
    // accompanies flag 34. Preserve that native packet verbatim; synthesizing
    // only StopDestroyBlock is insufficient on authoritative servers.
    if (pendingBreak && pendingBreak->nativeCommitArmed) {
        applySilentRotation(*pendingBreak);
        if (packet.inputFlags.test(34)) {
            // Native prediction has already removed a floor block locally by
            // this point, which clears collision flags before serialization.
            // The real held-mouse packet retains the pre-break collision state.
            packet.inputFlags.set(49, pendingBreak->lastInputFlags.test(49));
            packet.inputFlags.set(50, pendingBreak->lastInputFlags.test(50));

            // Preserve the native action sequence together with its opaque
            // transaction. With the GameMode context advanced every tick this
            // is the same path used by held-mouse mining.
            const auto hasFinalCrack = std::ranges::any_of(packet.blockActions,
                [](const PlayerBlockActionData& action) {
                    return action.type == PlayerActionType::CrackBlock;
                });
            if (!hasFinalCrack) {
                packet.blockActions.push_back({PlayerActionType::CrackBlock,
                    pendingBreak->target, pendingBreak->face});
            }
            pendingBreak->nativeCommitArmed = false;
            pendingBreak->commitSent = true;
            logF("[BlockBreakState] native transaction sent pos=({}, {}, {}) actions={}",
                pendingBreak->target.x, pendingBreak->target.y, pendingBreak->target.z,
                packet.blockActions.size());
        }
        return;
    }

    // Limiter owns block actions while its miner is active. Removing native
    // crosshair actions prevents the start/abort storms seen in the capture.
    packet.blockActions.clear();

    if (pendingAbort) {
        packet.blockActions.push_back(
            {PlayerActionType::AbortDestroyBlock, *pendingAbort, FacingID::Down});
        pendingAbort.reset();
    }

    if (pendingBreak) {
        pendingBreak->lastInputFlags = packet.inputFlags;
        applySilentRotation(*pendingBreak);
        if (pendingBreak->aimPacketsRemaining > 0) {
            --pendingBreak->aimPacketsRemaining;
        } else {
            if (pendingBreak->startPending) {
                packet.blockActions.push_back({PlayerActionType::StartDestroyBlock,
                    pendingBreak->target, pendingBreak->face});
                pendingBreak->startPending = false;
            }

            if (!pendingBreak->commitSent) {
                ++pendingBreak->crackTicks;
                if (pendingBreak->crackTicks >= pendingBreak->requiredTicks) {
                    pendingBreak->commitPending = true;
                    logF("[BlockBreakState] timed complete pos=({}, {}, {}) ticks={}/{}",
                        pendingBreak->target.x, pendingBreak->target.y, pendingBreak->target.z,
                        pendingBreak->crackTicks, pendingBreak->requiredTicks);
                }
            }

            packet.blockActions.push_back({PlayerActionType::CrackBlock,
                pendingBreak->target, pendingBreak->face});
        }
    }

    packet.inputFlags.set(35, !packet.blockActions.empty());
}

void flushCommit() {
    // Completion is injected into the outgoing PlayerAuthInput packet above.
}

} // namespace baritone::bedrock_block_breaking
