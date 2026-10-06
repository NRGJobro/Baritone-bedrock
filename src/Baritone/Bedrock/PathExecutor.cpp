#include "PathExecutor.h"
#include "../Core/NavigationPolicy.h"

#include "BedrockPhysics.h"
#include "BedrockBlockBreaking.h"
#include "BedrockWorld.h"
#include "MovementInput.h"
#include "../../SDK/MC.h"
#include "../../SDK/Client/Input/MoveInputComponent.h"
#include "../../SDK/World/Actor/Components/ActorHeadRotationComponent.h"
#include "../../SDK/World/Actor/Components/MobBodyRotationComponent.h"
#include "../../SDK/World/Actor/LocalPlayer.h"
#include "../../SDK/World/Actor/GameMode.h"
#include "../../SDK/World/Level/Level.h"
#include "../../SDK/World/Level/HitResult/HitResult.h"
#include "../../SDK/World/Level/HitResult/HitResultType.h"
#include "../../SDK/World/Block/Block.h"
#include "../../SDK/World/Block/BlockLegacy.h"
#include "../../SDK/World/Block/Material/Material.h"
#include "../../SDK/World/BlockSource.h"
#include "../../SDK/World/Inventory/Inventory.h"
#include "../../SDK/World/Inventory/PlayerInventory.h"
#include "../../SDK/World/Item/ItemStack.h"
#include "../../SDK/World/Item/Item.h"
#include "../../Utils/Logger.h"
#include "../../Utils/TimeUtils.h"

#include <numbers>

namespace baritone {
namespace {

constexpr std::array<glm::ivec3, 6> faceOffsets{{
    {0, -1, 0}, {0, 1, 0}, {0, 0, -1},
    {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}
}};

// GameMode::buildBlock serializes the current raycast contact into its item-use
// transaction on 1.26.52. Automated placement therefore has to provide the
// same solid support/face hit that an actual right click would have produced.
class PlacementHit {
    HitResult* hit = nullptr;
    std::optional<HitResult> saved;

public:
    PlacementHit(LocalPlayer* player, const glm::ivec3& support, const FacingID face) {
        const int faceIndex = static_cast<int>(face);
        auto* level = player == nullptr ? nullptr : player->getLevel();
        auto* wrapper = level == nullptr ? nullptr : level->getHitResultWrapper();
        if (wrapper == nullptr || faceIndex < 0 || faceIndex >= static_cast<int>(faceOffsets.size()))
            return;

        hit = &wrapper->hitResult;
        saved = *hit;
        const auto& normal = faceOffsets[faceIndex];
        hit->startPos = player->getPosition();
        hit->type = HitResultType::Tile;
        hit->facing = face;
        hit->blockPos = support;
        hit->pos = {
            support.x + 0.5f + normal.x * 0.5f,
            support.y + 0.5f + normal.y * 0.5f,
            support.z + 0.5f + normal.z * 0.5f
        };
        const glm::vec3 ray = hit->pos - hit->startPos;
        const float length = glm::length(ray);
        hit->rayDir = length > 0.0001f ? ray / length : glm::vec3{0.f, -1.f, 0.f};
    }

    PlacementHit(const PlacementHit&) = delete;
    PlacementHit& operator=(const PlacementHit&) = delete;

    ~PlacementHit() {
        if (hit != nullptr && saved)
            *hit = *saved;
    }

    explicit operator bool() const { return hit != nullptr; }
};

float horizontalDistance(const glm::vec3& left, const glm::vec3& right) {
    const float dx = left.x - right.x;
    const float dz = left.z - right.z;
    return std::sqrt(dx * dx + dz * dz);
}

struct SegmentProjection {
    float progress = 0.f;
    float lateralDistance = 0.f;
};

SegmentProjection projectOntoSegment(const glm::vec3& point, const BlockPos& from, const BlockPos& to) {
    const glm::vec2 start{from.x + 0.5f, from.z + 0.5f};
    const glm::vec2 end{to.x + 0.5f, to.z + 0.5f};
    const glm::vec2 position{point.x, point.z};
    const glm::vec2 segment = end - start;
    const float lengthSquared = glm::dot(segment, segment);
    if (lengthSquared <= 0.001f)
        return {0.f, glm::length(position - start)};
    const float progress = glm::dot(position - start, segment) / lengthSquared;
    // Deliberately project onto the infinite route line. Clamping this point to
    // the segment endpoint turns longitudinal overshoot into "lateral" error,
    // which made a perfectly straight sprint or drop look like it left the
    // path as soon as it travelled beyond the destination block centre.
    const glm::vec2 closest = start + segment * progress;
    return {progress, glm::length(position - closest)};
}

bool hasSafeSupport(const IWorld& world, const BlockPos& feet) {
    const auto support = world.getBlock(feet.offset(0, -1, 0));
    return support.loaded && support.solid && !support.hazard;
}

bool hasPlayerClearance(const IWorld& world, const BlockPos& feet, const bool allowLiquid) {
    const auto feetBlock = world.getBlock(feet);
    const auto headBlock = world.getBlock(feet.offset(0, 1, 0));
    return feetBlock.loaded && headBlock.loaded && !feetBlock.solid && !headBlock.solid &&
        !feetBlock.hazard && !headBlock.hazard &&
        (allowLiquid || (!feetBlock.liquid && !headBlock.liquid));
}

bool liquidAt(BlockSource* source, const glm::ivec3& pos) {
    if (source == nullptr) return false;
    auto* block = source->getBlock(pos);
    auto* legacy = block != nullptr ? block->getBlockLegacy() : nullptr;
    auto* material = legacy != nullptr ? legacy->getMaterial() : nullptr;
    return material != nullptr &&
        (material->type == MaterialType::Water || material->type == MaterialType::Lava);
}

FacingID facingFromPlayer(const glm::vec3& player, const BlockPos& block) {
    const glm::vec3 delta{player.x - (block.x + 0.5f), player.y - (block.y + 0.5f),
        player.z - (block.z + 0.5f)};
    const glm::vec3 magnitude{std::abs(delta.x), std::abs(delta.y), std::abs(delta.z)};
    if (magnitude.y >= magnitude.x && magnitude.y >= magnitude.z)
        return delta.y >= 0.f ? FacingID::Up : FacingID::Down;
    if (magnitude.x >= magnitude.z)
        return delta.x >= 0.f ? FacingID::East : FacingID::West;
    return delta.z >= 0.f ? FacingID::South : FacingID::North;
}

} // namespace

void PathExecutor::begin(std::vector<PathNode> newPath, const bool allowTerrainBreaking,
    const bool allowWater, const bool waterOnlyBridge, const bool onlyPlannedBreaks) {
    path = std::move(newPath);
    terrainBreakingAllowed = allowTerrainBreaking;
    plannedBreakingOnly = onlyPlannedBreaks;
    waterAllowed = allowWater;
    bridgeOverWaterOnly = waterOnlyBridge;
    index = path.size() > 1 ? 1 : path.size();
    lastProgressPosition = {};
    lastMotionPosition = {};
    motionSampleInitialized = false;
    ticksWithoutProgress = 0;
    ticksOutsidePath = 0;
    progressInitialized = false;
    controlledMovement = false;
    pathRotationActive = false;
    visualYawInitialized = false;
    cameraYawCaptured = false;
    renderRotationOverrideActive = false;
    lastRotationRenderMillis = 0;
    activeBreakIndex = static_cast<std::size_t>(-1);
    obstructionBreakTicks = 0;
    miningRecoveryTicks = 0;
    blockedTerrainTicks = 0;
    activeAscendIndex = static_cast<std::size_t>(-1);
    ascendJumpIssued = false;
    ascendWasAirborne = false;
    ascendLaunchTicks = 0;
    ascendRetries = 0;
    ascendRetryDelay = 0;
    resetParkourState();
}

bool PathExecutor::extendIfPrefix(const std::vector<PathNode>& candidate) {
    if (path.empty() || candidate.size() < path.size())
        return false;

    for (std::size_t candidateIndex = 0; candidateIndex < path.size(); ++candidateIndex) {
        if (path[candidateIndex].pos != candidate[candidateIndex].pos ||
            path[candidateIndex].movement != candidate[candidateIndex].movement)
            return false;
    }

    path.insert(path.end(), candidate.begin() + static_cast<std::ptrdiff_t>(path.size()),
        candidate.end());
    return true;
}

void PathExecutor::updateCapabilities(const bool allowTerrainBreaking,
    const bool allowWater, const bool waterOnlyBridge,
    const bool onlyPlannedBreaks) {
    terrainBreakingAllowed = allowTerrainBreaking;
    plannedBreakingOnly = onlyPlannedBreaks;
    waterAllowed = allowWater;
    bridgeOverWaterOnly = waterOnlyBridge;
}

ExecutionStatus PathExecutor::tick(LocalPlayer* player, const ExecutionOptions& options) {
    if (player == nullptr)
        return ExecutionStatus::NoPlayer;

    rotationSmoothness = std::clamp(options.rotationSmoothness, 0.25f, 10.f);

    if (index >= path.size()) {
        clearInput(player);
        restoreMiningHotbar(player);
        return ExecutionStatus::Arrived;
    }

    // Every movement below must travel through Minecraft's real input
    // component. Do not report a live executor when the current ECS ABI could
    // not resolve that component: the path renderer would advance while the
    // player received no vanilla W/A/S/D state at all.
    if (player->tryGet<MoveInputComponent>() == nullptr) {
        lastFailureReason = "vanilla movement input unavailable";
        return ExecutionStatus::Stuck;
    }

    const auto feet = player->getFeetPosition();
    glm::vec3 measuredMotion{};
    // StateVector::posPrev is already synchronized to pos by the time this
    // callback runs on 1.26.52, so subtracting the two always reports zero.
    // Measure the actor's real per-tick displacement ourselves. This is only
    // observation: Bedrock remains responsible for applying all movement.
    if (motionSampleInitialized)
        measuredMotion = feet - lastMotionPosition;
    lastMotionPosition = feet;
    motionSampleInitialized = true;
    // Ignore teleports/server corrections. Ordinary horizontal movement is
    // substantially below one block per simulation tick.
    if (!std::isfinite(measuredMotion.x) || !std::isfinite(measuredMotion.y) ||
        !std::isfinite(measuredMotion.z) ||
        glm::length(glm::vec2{measuredMotion.x, measuredMotion.z}) > 1.25f)
        measuredMotion = {};
    // Java Baritone advances movements from playerFeet(), not from a predicted
    // future position. The small positive Y allowance mirrors its handling of
    // tiny Bedrock/Java standing-height inaccuracies.
    const BlockPos playerFeetBlock{
        static_cast<int>(std::floor(feet.x)),
        static_cast<int>(std::floor(feet.y + 0.1251f)),
        static_cast<int>(std::floor(feet.z))
    };
    const glm::ivec3 feetCell{
        static_cast<int>(std::floor(feet.x)),
        static_cast<int>(std::floor(feet.y)),
        static_cast<int>(std::floor(feet.z))};
    const bool inWaterBlocks = liquidAt(MC::getRegion(), feetCell) ||
        liquidAt(MC::getRegion(), feetCell + glm::ivec3{0, 1, 0});

    if (!progressInitialized) {
        lastProgressPosition = feet;
        progressInitialized = true;
    }

    // Before opening a vertical shaft, move to the center of the current
    // block. A path node is considered reached anywhere inside its voxel, but
    // mining the floor from an edge can leave part of the hitbox supported and
    // prevent the intended straight fall.
    if (terrainBreakingAllowed && index > 0 && index < path.size() &&
        path[index].movement == MovementType::BreakDown && player->isOnGround() &&
        playerFeetBlock != path[index].pos) {
        if (!validBreakDownGroundCell(
            path[index - 1].pos, path[index].pos, playerFeetBlock)) {
            clearInput(player);
            lastFailureReason = "not positioned above vertical shaft";
            return ExecutionStatus::OffPath;
        }
        const bool enteringShaft = requiresBreakDownEntryCentering(
            path[index - 1].movement);
        if (enteringShaft) {
        const glm::vec2 center{path[index - 1].pos.x + 0.5f,
            path[index - 1].pos.z + 0.5f};
        const glm::vec2 centerError = center - glm::vec2{feet.x, feet.z};
        const glm::vec2 horizontalMotion{measuredMotion.x, measuredMotion.z};
        const float centerDistance = glm::length(centerError);
        const float horizontalSpeed = glm::length(horizontalMotion);
        // Position alone is not enough: arriving at the center with residual
        // walking momentum used to start mining and then carry the player off
        // the block. Settle both position and velocity while sneaking.
        const bool safelyCentered = bedrock_physics::shaftFootprintCentered(
            centerError.x, centerError.y);
        if (!safelyCentered || horizontalSpeed > 0.025f) {
            glm::vec2 correction{};
            if (!safelyCentered) {
                const glm::vec2 towardCenter = centerError / centerDistance;
                const float inwardSpeed = glm::dot(horizontalMotion, towardCenter);
                correction = towardCenter *
                    bedrock_physics::shaftCenterInput(centerDistance, inwardSpeed);
            }
            const float yaw = player->getRotation().y * std::numbers::pi_v<float> / 180.f;
            const glm::vec2 forward{-std::sin(yaw), std::cos(yaw)};
            const glm::vec2 right{forward.y, -forward.x};
            const float forwardAmount = std::clamp(glm::dot(forward, correction), -0.32f, 0.32f);
            const float strafeAmount = std::clamp(glm::dot(right, correction), -0.32f, 0.32f);
            if (const auto input = player->tryGet<MoveInputComponent>()) {
                // This correction is already camera-relative. Preserve the
                // native movement-mode flags so Bedrock interprets it exactly
                // like every other path command.
                const auto safeMovement = movement_input::sanitize({strafeAmount, forwardAmount});
                const glm::vec2 movement{safeMovement.x, safeMovement.y};
                const auto directions = movement_input::directions(safeMovement);
                input->move = movement;
                input->inputState.analogMoveVector = movement;
                input->rawInputState.analogMoveVector = movement;
                input->inputState.up = input->rawInputState.up = directions.up;
                input->inputState.down = input->rawInputState.down = directions.down;
                input->inputState.left = input->rawInputState.left = directions.left;
                input->inputState.right = input->rawInputState.right = directions.right;
                input->inputState.upLeft = input->rawInputState.upLeft = input->inputState.up && input->inputState.left;
                input->inputState.upRight = input->rawInputState.upRight = input->inputState.up && input->inputState.right;
                input->inputState.downLeft = input->rawInputState.downLeft = input->inputState.down && input->inputState.left;
                input->inputState.downRight = input->rawInputState.downRight = input->inputState.down && input->inputState.right;
                input->inputState.sprintDown = input->rawInputState.sprintDown = false;
                input->inputState.jumpDown = input->rawInputState.jumpDown = false;
                input->inputState.jumpInputCurrentlyDown = false;
                input->rawInputState.jumpInputCurrentlyDown = false;
                input->inputState.sneakDown = input->rawInputState.sneakDown = true;
                input->inputState.sneakInputCurrentlyDown = true;
                input->rawInputState.sneakInputCurrentlyDown = true;
                input->sprinting = false;
                input->jumping = false;
                input->sneaking = true;
                input->persistSneak = false;
                input->wantDown = false;
                input->moveInputStateLocked = false;
                captureInputCommand(input);
                controlledMovement = true;
            }
            ticksWithoutProgress = 0;
            return ExecutionStatus::Running;
        }
        }
    }

    // A mining process owns this policy for the lifetime of its route. Inspect
    // the real swept 1x2 clearance on every transition instead of trusting the
    // movement label chosen during planning: falling blocks, world updates, or
    // a tight diagonal can otherwise turn an ordinary node into an obstruction.
    if (terrainBreakingAllowed && player->isOnGround() &&
        index < path.size() && MC::getRegion() != nullptr) {
        const BedrockWorld world(MC::getRegion());
        std::vector<BlockPos> clearanceCells;
        clearanceCells.reserve(8);
        const auto addCell = [&](const BlockPos& pos) {
            if (std::ranges::find(clearanceCells, pos) == clearanceCells.end())
                clearanceCells.push_back(pos);
        };
        const auto addColumn = [&](const BlockPos& feetPos) {
            // Clear the visible upper block first, then the feet block below
            // it. This gives descending tunnel work a natural top-to-bottom
            // order while still clearing the complete two-block column.
            addCell(feetPos.offset(0, 1, 0));
            addCell(feetPos);
        };

        // If the actor is already clipping a newly placed/fallen ceiling,
        // clear that first before attempting any horizontal movement.
        addCell(playerFeetBlock.offset(0, 1, 0));

        // Recovery steering may approach the active path node from a physical
        // block other than its planned parent. Clear the real first step too,
        // otherwise the executor can push into that wall until stall recovery.
        const int actualDx = std::clamp(path[index].pos.x - playerFeetBlock.x, -1, 1);
        const int actualDz = std::clamp(path[index].pos.z - playerFeetBlock.z, -1, 1);
        const int actualDistance = std::max(
            std::abs(path[index].pos.x - playerFeetBlock.x),
            std::abs(path[index].pos.z - playerFeetBlock.z));
        if (actualDistance > 1)
            addColumn(playerFeetBlock.offset(actualDx, 0, actualDz));
        if (actualDx != 0 && actualDz != 0) {
            addColumn(playerFeetBlock.offset(actualDx, 0, 0));
            addColumn(playerFeetBlock.offset(0, 0, actualDz));
        }
        if (path[index].pos.y > playerFeetBlock.y)
            addCell(playerFeetBlock.offset(0, 2, 0));

        if (index > 0) {
            const auto& source = path[index - 1].pos;
            const auto movement = path[index].movement;
            const int dx = std::clamp(path[index].pos.x - source.x, -1, 1);
            const int dz = std::clamp(path[index].pos.z - source.z, -1, 1);
            const int distance = std::max(
                std::abs(path[index].pos.x - source.x),
                std::abs(path[index].pos.z - source.z));

            if (movement == MovementType::Ascend || movement == MovementType::BreakAscend)
                addCell(source.offset(0, 2, 0));

            if (movement == MovementType::Descend || movement == MovementType::BreakDescend ||
                movement == MovementType::Fall || movement == MovementType::WaterDrop) {
                // A descent begins with a horizontal step at the source Y. Its
                // current-height feet/head column must be mined before the
                // actor can fall into the lower destination column. Keep this
                // ahead of the destination in clearanceCells: selecting the
                // lower block first looks like the miner is breaking through
                // an unseen wall below the visible tunnel opening.
                addColumn(source.offset(dx, 0, dz));
            }

            // The player's width sweeps both orthogonal columns during a
            // diagonal. They must be cleared at feet and head height as well.
            if (dx != 0 && dz != 0) {
                addColumn(source.offset(dx, 0, 0));
                addColumn(source.offset(0, 0, dz));
            }
            for (int step = 1; step < distance; ++step)
                addColumn(source.offset(dx * step, 0, dz * step));
        }

        // Check the destination only after the immediate/current-height work
        // cells above. This makes descending routes clear from the player's
        // visible frontier toward the lower landing instead of bottom-to-top.
        addColumn(path[index].pos);

        std::optional<BlockPos> obstruction;
        bool foundUnbreakable = false;
        bool foundUnsafe = false;
        const auto neededAsFutureSupport = [&](const BlockPos& candidate) {
            const std::size_t first = index > 0 ? index - 1 : index;
            for (std::size_t cursor = first; cursor < path.size(); ++cursor) {
                if (cursor + 1 == index && path[index].movement == MovementType::BreakDown)
                    continue;
                if (path[cursor].pos.offset(0, -1, 0) == candidate)
                    return true;
            }
            return false;
        };
        for (const auto& candidate : clearanceCells) {
            const auto state = world.getBlock(candidate);
            if (!state.loaded || state.hazard || (!waterAllowed && state.liquid)) {
                foundUnsafe = true;
                break;
            }
            if (state.solid) {
                if (plannedBreakingOnly && (index == 0 ||
                    !MovementGenerator::isPlannedBreakCell(path[index - 1].pos,
                        path[index].pos, path[index].movement, candidate))) {
                    foundUnsafe = true;
                    break;
                }
                // Never let recovery clearance destroy a block that this same
                // route expects to stand or jump on later. The controller also
                // rejects planned conflicts; this is the last-moment guard for
                // physical/path-index drift.
                if (neededAsFutureSupport(candidate)) {
                    foundUnsafe = true;
                    break;
                }
                if (!state.breakable) {
                    foundUnbreakable = true;
                    break;
                }
                // Revalidate immediately before every destroy call. Flowing
                // water may have reached a neighbor after A* built the route.
                if (MovementGenerator::wouldExposeLiquid(world, candidate)) {
                    foundUnsafe = true;
                    break;
                }
                obstruction = candidate;
                break;
            }
        }

        if (foundUnbreakable) {
            if (obstructionBreakTicks > 0 && player->getGameMode() != nullptr)
                bedrock_block_breaking::stop(player, {activeBreakPos.x, activeBreakPos.y, activeBreakPos.z});
            clearInput(player);
            activeBreakIndex = static_cast<std::size_t>(-1);
            obstructionBreakTicks = 0;
            // Do not throw away the route on the first transient/stale block
            // classification after a break. If the obstruction is genuinely
            // permanent, the bounded stall path performs one normal recovery.
            if (++blockedTerrainTicks > 80) {
                lastFailureReason = "unbreakable obstruction";
                return ExecutionStatus::Stuck;
            }
            return ExecutionStatus::Running;
        }

        if (foundUnsafe) {
            if (obstructionBreakTicks > 0 && player->getGameMode() != nullptr)
                bedrock_block_breaking::stop(player, {activeBreakPos.x, activeBreakPos.y, activeBreakPos.z});
            clearInput(player);
            activeBreakIndex = static_cast<std::size_t>(-1);
            obstructionBreakTicks = 0;
            // The world changed underneath the route. Never walk or mine into
            // a hazardous block; let the controller replan with the current
            // hazard map instead.
            lastFailureReason = "unsafe obstruction";
            return ExecutionStatus::OffPath;
        }

        if (obstruction) {
            blockedTerrainTicks = 0;
            auto* gameMode = player->getGameMode();
            if (gameMode == nullptr)
                return ExecutionStatus::NoPlayer;
            if (activeBreakIndex != index || activeBreakPos != *obstruction) {
                if (obstructionBreakTicks > 0)
                    bedrock_block_breaking::stop(player, {activeBreakPos.x, activeBreakPos.y, activeBreakPos.z});
                activeBreakIndex = index;
                activeBreakPos = *obstruction;
                obstructionBreakTicks = 0;
                selectBestTool(player, *obstruction);
            }

            if (const auto input = player->tryGet<MoveInputComponent>()) {
                const bool miningStraightDown = index < path.size() &&
                    path[index].movement == MovementType::BreakDown;
                const bool sneakWasDown = input->rawInputState.sneakInputCurrentlyDown;
                input->move = {};
                input->inputState.analogMoveVector = {};
                input->rawInputState.analogMoveVector = {};
                input->inputState.up = input->inputState.down = false;
                input->inputState.left = input->inputState.right = false;
                input->rawInputState.up = input->rawInputState.down = false;
                input->rawInputState.left = input->rawInputState.right = false;
                input->inputState.upLeft = input->inputState.upRight = false;
                input->inputState.downLeft = input->inputState.downRight = false;
                input->rawInputState.upLeft = input->rawInputState.upRight = false;
                input->rawInputState.downLeft = input->rawInputState.downRight = false;
                input->inputState.sprintDown = input->rawInputState.sprintDown = false;
                input->inputState.jumpDown = input->rawInputState.jumpDown = false;
                input->inputState.jumpInputCurrentlyDown = false;
                input->rawInputState.jumpInputCurrentlyDown = false;
                input->inputState.jumpInputWasPressed = input->rawInputState.jumpInputWasPressed = false;
                input->inputState.jumpInputWasReleased = input->rawInputState.jumpInputWasReleased = false;
                input->sprinting = false;
                input->jumping = false;
                input->inputState.sneakDown = input->rawInputState.sneakDown = miningStraightDown;
                input->inputState.sneakInputCurrentlyDown = miningStraightDown;
                input->rawInputState.sneakInputCurrentlyDown = miningStraightDown;
                input->inputState.sneakInputWasPressed =
                    input->rawInputState.sneakInputWasPressed = miningStraightDown && !sneakWasDown;
                input->inputState.sneakInputWasReleased =
                    input->rawInputState.sneakInputWasReleased = !miningStraightDown && sneakWasDown;
                input->sneaking = miningStraightDown;
                input->persistSneak = false;
                // postTick reapplies the cached command after Bedrock's native
                // tick. Cache this zero-motion mining state or it will replay
                // the previous walking/centering command while the floor is
                // being broken.
                captureInputCommand(input);
                controlledMovement = true;
            }

            const glm::ivec3 target{obstruction->x, obstruction->y, obstruction->z};
            const auto playerPosition = player->getPosition();
            const auto face = facingFromPlayer(playerPosition, *obstruction);
            bedrock_block_breaking::tick(player, target, face, playerPosition);
            ++obstructionBreakTicks;
            // Breaking and newly opened descents can shift the actor away from
            // the exact block-center rail. Keep a short recovery window after
            // the obstruction disappears instead of invalidating that route.
            miningRecoveryTicks = 30;
            ticksWithoutProgress = 0;
            ticksOutsidePath = 0;
            if (obstructionBreakTicks > 240) {
                bedrock_block_breaking::stop(player, target);
                lastFailureReason = "breaking timed out";
                return ExecutionStatus::Stuck;
            }
            return ExecutionStatus::Running;
        }

        if (obstructionBreakTicks > 0 && player->getGameMode() != nullptr)
            bedrock_block_breaking::stop(player, {activeBreakPos.x, activeBreakPos.y, activeBreakPos.z});
        activeBreakIndex = static_cast<std::size_t>(-1);
        obstructionBreakTicks = 0;
        blockedTerrainTicks = 0;
        if (miningRecoveryTicks > 0)
            --miningRecoveryTicks;
    }

    // A jump can land one walking node past its target. Resolve that exact,
    // supported landing before corridor checks and the missed-jump detector.
    // Otherwise a successful landing is classified as Stuck on this tick.
    if (player->isOnGround() && MC::getRegion() != nullptr &&
        ((activeParkourIndex == index && parkourJumpIssued && parkourWasAirborne) ||
            (activeAscendIndex == index && ascendJumpIssued && ascendWasAirborne))) {
        const BedrockWorld world(MC::getRegion());
        const auto landed = jumpLandingIndex(path, index, playerFeetBlock, world);
        if (landed < path.size()) {
            index = landed;
            ticksWithoutProgress = 0;
            ticksOutsidePath = 0;
            lastProgressPosition = feet;
            resetParkourState();
        } else if (repairJumpLanding(path, index, playerFeetBlock, world)) {
            // Keep the remaining route and steer over this verified walking
            // edge now, rather than returning Stuck and restarting A*.
            ticksWithoutProgress = 0;
            ticksOutsidePath = 0;
            lastProgressPosition = feet;
            resetParkourState();
        }
    }

    // Terrain removal can drop or nudge the actor onto a later route block in
    // one physics tick. Resynchronize exact physical matches before applying
    // lateral corridor rejection, otherwise a valid descent is thrown away.
    if (terrainBreakingAllowed && player->isOnGround() && index + 1 < path.size()) {
        const std::size_t resyncEnd = std::min(path.size() - 1, index + 8);
        std::size_t matched = path.size();
        for (std::size_t cursor = index + 1; cursor <= resyncEnd; ++cursor) {
            if (path[cursor].pos == playerFeetBlock) {
                matched = cursor;
                break;
            }
        }
        if (matched < path.size()) {
            // Keep the physically matched node active. It still needs normal
            // centre/turn completion below; consuming it here made mining turn
            // toward the following segment from the edge of the block.
            index = matched;
            lastProgressPosition = feet;
            ticksWithoutProgress = 0;
            ticksOutsidePath = 0;
            resetParkourState();
        }
    }

    // Baritone's short-fall override allows momentum to carry a fall through
    // as many as two following straight traverses. Splice only when the real
    // player-feet block exactly matches that verified continuation.
    if (player->isOnGround() && index > 0 && index < path.size() &&
        (path[index].movement == MovementType::Descend || path[index].movement == MovementType::Fall)) {
        const auto& source = path[index - 1].pos;
        const int fallX = std::clamp(path[index].pos.x - source.x, -1, 1);
        const int fallZ = std::clamp(path[index].pos.z - source.z, -1, 1);
        std::size_t matchedIndex = path.size();
        const std::size_t lastCandidate = std::min(path.size() - 1, index + 2);
        for (std::size_t candidate = index + 1; candidate <= lastCandidate; ++candidate) {
            const auto& previous = path[candidate - 1].pos;
            const auto& current = path[candidate];
            if (current.movement != MovementType::Traverse || current.pos.y != path[index].pos.y ||
                current.pos.x - previous.x != fallX || current.pos.z - previous.z != fallZ)
                break;
            if (playerFeetBlock == current.pos)
                matchedIndex = candidate;
        }
        if (matchedIndex < path.size()) {
            index = matchedIndex + 1;
            resetParkourState();
            lastProgressPosition = feet;
            ticksWithoutProgress = 0;
            ticksOutsidePath = 0;
            if (index >= path.size()) {
                clearInput(player);
                restoreMiningHotbar(player);
                return ExecutionStatus::Arrived;
            }
        }
    }

    // Freeze completion while beside the active route. Predictive progress is
    // useful only inside the movement corridor; outside it, consuming nodes
    // makes the visual path run away from the player during recovery.
    bool outsideActiveCorridor = false;
    if (player->isOnGround() && index > 0 && index < path.size()) {
        const auto activeProjection = projectOntoSegment(feet, path[index - 1].pos, path[index].pos);
        // Water currents can hold the body against the side of a one-block
        // column while the feet are already supported at its bottom.  Keep
        // node completion active across the complete water block instead of
        // freezing it at the narrower ground-movement tolerance.
        const float corridorWidth = path[index].movement == MovementType::Swim ? 1.05f :
            (terrainBreakingAllowed ? (miningRecoveryTicks > 0 ? 1.35f : 1.10f) : 0.82f);
        outsideActiveCorridor = activeProjection.lateralDistance > corridorWidth;
    }

    while (!outsideActiveCorridor && index < path.size()) {
        const auto& node = path[index];
        // Match Baritone's default success rule: the physical player-feet block
        // must be the movement destination. Do not erase nodes based on a
        // distance radius or momentum prediction.
        bool reached = playerFeetBlock == node.pos &&
            (player->isOnGround() || node.movement == MovementType::Swim ||
                (node.movement == MovementType::WaterDrop && inWaterBlocks));
        if (!reached && node.movement == MovementType::WaterDrop && inWaterBlocks) {
            const float dx = feet.x - (static_cast<float>(node.pos.x) + 0.5f);
            const float dz = feet.z - (static_cast<float>(node.pos.z) + 0.5f);
            reached = dx * dx + dz * dz <= 0.90f * 0.90f &&
                feet.y <= static_cast<float>(node.pos.y) + 1.50f;
        }
        if (!reached && node.movement == MovementType::Swim) {
            const float dx = feet.x - (static_cast<float>(node.pos.x) + 0.5f);
            const float dz = feet.z - (static_cast<float>(node.pos.z) + 0.5f);
            // Surface buoyancy lifts the feet above the integer route cell.
            // Do not consume ascent nodes while still a full block below them.
            reached = dx * dx + dz * dz <= 0.62f * 0.62f &&
                feet.y >= static_cast<float>(node.pos.y) - 0.15f &&
                feet.y <= static_cast<float>(node.pos.y) + 1.05f;
        }
        // A Bedrock fall can land past the destination block centre even after
        // forward input is released. Complete the fall from the supported
        // physical landing corridor so the next movement takes control; never
        // steer backward toward the top of the drop.
        if (!reached && index > 0 && player->isOnGround() && MC::getRegion() != nullptr &&
            (node.movement == MovementType::Descend || node.movement == MovementType::Fall) &&
            std::abs(feet.y - static_cast<float>(node.pos.y)) <= 0.85f) {
            const auto landing = projectOntoSegment(feet, path[index - 1].pos, node.pos);
            const BedrockWorld world(MC::getRegion());
            reached = validSupportedFallLanding(hasSafeSupport(world, playerFeetBlock),
                feet.y - static_cast<float>(node.pos.y), landing.progress,
                landing.lateralDistance);
        }

        // A sequence of drops is one continuous physical movement. Bedrock
        // momentum can carry the player onto a later landing node, so accept a
        // supported future fall landing instead of steering back to the first
        // drop and declaring the route invalid.
        if (!reached && index > 0 && player->isOnGround() && MC::getRegion() != nullptr &&
            (node.movement == MovementType::Descend || node.movement == MovementType::Fall)) {
            const auto& source = path[index - 1].pos;
            const int routeX = std::clamp(node.pos.x - source.x, -1, 1);
            const int routeZ = std::clamp(node.pos.z - source.z, -1, 1);
            const BedrockWorld world(MC::getRegion());
            std::size_t chainIndex = index;
            const std::size_t chainEnd = std::min(path.size() - 1, index + 4);
            for (std::size_t candidate = index + 1; candidate <= chainEnd; ++candidate) {
                const auto& previous = path[candidate - 1].pos;
                const auto& landing = path[candidate];
                if ((landing.movement != MovementType::Descend && landing.movement != MovementType::Fall) ||
                    landing.pos.x - previous.x != routeX || landing.pos.z - previous.z != routeZ ||
                    landing.pos.y > previous.y)
                    break;
                if (playerFeetBlock == landing.pos && hasSafeSupport(world, landing.pos)) {
                    chainIndex = candidate;
                    break;
                }
            }
            if (chainIndex != index) {
                index = chainIndex + 1;
                lastProgressPosition = feet;
                ticksWithoutProgress = 0;
                ticksOutsidePath = 0;
                if (index >= path.size()) {
                    clearInput(player);
                    restoreMiningHotbar(player);
                    return ExecutionStatus::Arrived;
                }
                continue;
            }
        }

        // A parkour landing one block beyond the intended destination is only
        // accepted when it is actually supported and still tightly aligned.
        // This is recovery from a safe overshoot, not predictive completion.
        if (!reached && node.movement == MovementType::Parkour && index > 0 && player->isOnGround() &&
            MC::getRegion() != nullptr) {
            const auto& sourceNode = path[index - 1];
            const auto actual = projectOntoSegment(feet, sourceNode.pos, node.pos);
            const int stepX = std::clamp(node.pos.x - sourceNode.pos.x, -1, 1);
            const int stepZ = std::clamp(node.pos.z - sourceNode.pos.z, -1, 1);
            const BlockPos safeOvershoot = node.pos.offset(stepX, 0, stepZ);
            const BedrockWorld world(MC::getRegion());
            reached = playerFeetBlock == safeOvershoot && hasSafeSupport(world, safeOvershoot) &&
                actual.progress >= 1.f && actual.progress <= 1.45f && actual.lateralDistance <= 0.35f;
        }

        // Entering a waypoint's block is not enough when the mining route is
        // about to turn. Reach its centreline first so the player's full-width
        // collision box clears the inside corner before steering rotates to
        // the next segment.
        if (reached && player->isOnGround() &&
            index > 0 && index + 1 < path.size() &&
            (node.movement == MovementType::Traverse ||
                node.movement == MovementType::Diagonal ||
                node.movement == MovementType::BreakTraverse)) {
            const auto& previous = path[index - 1].pos;
            const auto& next = path[index + 1].pos;
            const int incomingX = std::clamp(node.pos.x - previous.x, -1, 1);
            const int incomingZ = std::clamp(node.pos.z - previous.z, -1, 1);
            const int outgoingX = std::clamp(next.x - node.pos.x, -1, 1);
            const int outgoingZ = std::clamp(next.z - node.pos.z, -1, 1);
            const bool turnWaypoint = incomingX != outgoingX || incomingZ != outgoingZ ||
                next.y != node.pos.y;
            if (turnWaypoint) {
                const float centreX = static_cast<float>(node.pos.x) + 0.5f;
                const float centreZ = static_cast<float>(node.pos.z) + 0.5f;
                const float offsetX = feet.x - centreX;
                const float offsetZ = feet.z - centreZ;
                reached = offsetX * offsetX + offsetZ * offsetZ <= 0.18f * 0.18f;
            }
        }

        // Every transition into BreakDown, regardless of how its source was
        // reached, must finish at rest over the exact source centre. Keep the
        // intact floor in control until this is true; only the following tick
        // is then allowed to select and mine the block directly underneath.
        if (reached && player->isOnGround() &&
            requiresBreakDownEntryCentering(node.movement) && index + 1 < path.size() &&
            path[index + 1].movement == MovementType::BreakDown) {
            const float centreX = static_cast<float>(node.pos.x) + 0.5f;
            const float centreZ = static_cast<float>(node.pos.z) + 0.5f;
            const float offsetX = feet.x - centreX;
            const float offsetZ = feet.z - centreZ;
            const float horizontalSpeed = glm::length(
                glm::vec2{measuredMotion.x, measuredMotion.z});
            reached = bedrock_physics::shaftFootprintCentered(offsetX, offsetZ) &&
                horizontalSpeed <= 0.025f;
        }

        if (!reached)
            break;
        ++index;
    }

    if (index >= path.size()) {
        clearInput(player);
        restoreMiningHotbar(player);
        return ExecutionStatus::Arrived;
    }

    // If momentum carried the player completely beyond an unconsumed node,
    // stop before issuing another command. In particular, never keep treating
    // a missed descent landing as an approach to the edge above it.
    if (player->isOnGround() && index > 0) {
        const auto missed = projectOntoSegment(feet, path[index - 1].pos, path[index].pos);
        const auto movement = path[index].movement;
        const bool drop = movement == MovementType::Descend || movement == MovementType::Fall;
        if (missed.progress > (drop ? 2.25f : 1.35f)) {
            clearInput(player);
            lastFailureReason = "overshot movement node";
            return ExecutionStatus::OffPath;
        }
    }

    // Do not teleport progress to a merely nearby future segment. Give small
    // deviations a brief correction window, then let A* calculate a genuine
    // route from the player's real block if the corridor cannot be rejoined.
    if (outsideActiveCorridor) {
        // Ordinary routes receive enough time to center from any point within
        // their starting block. Mining gets additional hysteresis because a
        // broken support or downward transition legitimately displaces it.
        ++ticksOutsidePath;
        // Mining can clear the real recovery step toward its active node. Do
        // not discard the complete A* result for lateral drift after every
        // block; the independent no-progress detector still replans if this
        // recovery genuinely stalls. Ordinary navigation stays strict.
        if (!terrainBreakingAllowed && ticksOutsidePath > 4) {
            clearInput(player);
            lastFailureReason = "outside movement corridor";
            return ExecutionStatus::OffPath;
        }
    } else {
        ticksOutsidePath = 0;
    }

    // Airborne ticks are active progress for falls and jumps. Counting them as
    // stagnation causes false recovery on two-block drops near the jump apex.
    const bool preparingBridge = index < path.size() && path[index].movement == MovementType::Bridge;
    const bool waterRoute = path[index].movement == MovementType::Swim ||
        (index > 0 && BedrockWorld(MC::getRegion()).getBlock(path[index - 1].pos).liquid);
    if (preparingBridge)
        ticksWithoutProgress = 0;
    if (!player->isOnGround() && !inWaterBlocks && !waterRoute) {
        lastProgressPosition = feet;
        ticksWithoutProgress = 0;
    } else if (bedrock_physics::madeWaterProgress(horizontalDistance(feet, lastProgressPosition),
        feet.y - lastProgressPosition.y, index > 0 && path[index].pos.y > path[index - 1].pos.y &&
            path[index].movement == MovementType::Swim) ||
        (!inWaterBlocks && !waterRoute && std::abs(feet.y - lastProgressPosition.y) > 0.35f)) {
        lastProgressPosition = feet;
        ticksWithoutProgress = 0;
    } else if (!preparingBridge && ++ticksWithoutProgress > 80) {
        clearInput(player);
        lastFailureReason = "no movement progress";
        return ExecutionStatus::Stuck;
    }

    const auto& node = path[index];

    const bool isAscending = node.movement == MovementType::Ascend ||
        node.movement == MovementType::BreakAscend ||
        node.movement == MovementType::BuildAscend;
    if (isAscending) {
        if (activeAscendIndex != index) {
            activeAscendIndex = index;
            ascendJumpIssued = false;
            ascendWasAirborne = false;
            ascendLaunchTicks = 0;
            ascendRetries = 0;
            ascendRetryDelay = 0;
        } else {
            if (ascendRetryDelay > 0)
                --ascendRetryDelay;
            if (ascendJumpIssued) {
                ++ascendLaunchTicks;
                if (!player->isOnGround())
                    ascendWasAirborne = true;
                // A low/early launch can bump the head and land back in the
                // source voxel. Re-arm this same ascent after a brief
                // centering window instead of throwing away the whole path.
                if (bedrock_physics::shouldRetryAscent(player->isOnGround(), feet.y,
                    static_cast<float>(node.pos.y), ascendWasAirborne, ascendLaunchTicks)) {
                    if (++ascendRetries > 3) {
                        clearInput(player);
                        lastFailureReason = "ascent retries exhausted";
                        return ExecutionStatus::Stuck;
                    }
                    ascendJumpIssued = false;
                    ascendWasAirborne = false;
                    ascendLaunchTicks = 0;
                    ascendRetryDelay = 3;
                    ticksWithoutProgress = 0;
                }
            }
        }
    } else {
        activeAscendIndex = static_cast<std::size_t>(-1);
        ascendJumpIssued = false;
        ascendWasAirborne = false;
        ascendLaunchTicks = 0;
        ascendRetries = 0;
        ascendRetryDelay = 0;
    }
    if (isAscending && inWaterBlocks)
        ascendJumpIssued = true; // swim into the bank; never retreat to a grounded takeoff point

    // A break can invalidate the support relationship that was true when the
    // path was planned. Never keep jumping from a missing/stale support block;
    // hand the route back to A* so it can choose a valid approach.
    // During the jump the rounded feet cell can temporarily advance above the
    // real support block. Only validate support while the actor is grounded;
    // airborne ticks belong to the same active jump and must not trigger a
    // false off-path recovery.
    if (terrainBreakingAllowed && isAscending && player->isOnGround() &&
        MC::getRegion() != nullptr) {
        const BedrockWorld world(MC::getRegion());
        const bool currentSupported = hasSafeSupport(world, playerFeetBlock);
        const bool destinationSupported = hasSafeSupport(world, node.pos);
        if (!currentSupported || (node.movement != MovementType::BuildAscend && !destinationSupported)) {
            clearInput(player);
            lastFailureReason = "missing ascent support";
            return ExecutionStatus::OffPath;
        }
    }

    // Revalidate the complete 1x2 player column immediately before movement.
    // This protects regular navigation from changed blocks and prevents the
    // executor from repeatedly walking or jumping into a low ceiling. Mining
    // transitions reach here only after both obstructions have been cleared.
    if (MC::getRegion() != nullptr) {
        const BedrockWorld world(MC::getRegion());
        bool clear = hasPlayerClearance(world, node.pos, waterAllowed);
        if (node.movement == MovementType::WaterDrop && index > 0 && player->isOnGround())
            clear = clear && MovementGenerator::canDropToWater(world, path[index - 1].pos, node.pos);
        if (clear && player->isOnGround() && index > 0 && (node.movement == MovementType::Diagonal ||
            node.movement == MovementType::Swim)) {
            const auto& source = path[index - 1].pos;
            const int dx = std::clamp(node.pos.x - source.x, -1, 1);
            const int dz = std::clamp(node.pos.z - source.z, -1, 1);
            clear = hasPlayerClearance(world, source.offset(dx, 0, 0), waterAllowed) &&
                hasPlayerClearance(world, source.offset(0, 0, dz), waterAllowed);
        }
        if (clear && player->isOnGround() && index > 0 &&
            (node.movement == MovementType::Ascend || node.movement == MovementType::BreakAscend ||
                node.movement == MovementType::BuildAscend)) {
            // A one-block jump sweeps the player's head through the cell above
            // the source before the feet arrive at the raised destination.
            clear = hasPlayerClearance(world, path[index - 1].pos.offset(0, 1, 0), waterAllowed);
        }
        if (clear && player->isOnGround() && index > 0 &&
            (node.movement == MovementType::Descend || node.movement == MovementType::BreakDescend ||
                node.movement == MovementType::Fall || node.movement == MovementType::WaterDrop)) {
            const auto& source = path[index - 1].pos;
            const int dx = std::clamp(node.pos.x - source.x, -1, 1);
            const int dz = std::clamp(node.pos.z - source.z, -1, 1);
            clear = hasPlayerClearance(world, source.offset(dx, 0, dz), waterAllowed);
        }
        if (!clear) {
            clearInput(player);
            // A changed landing must not start A* from an unsupported mid-air
            // position. Release unsafe input but retain the route until the
            // actor is grounded, when clearance/recovery can be evaluated.
            if (!player->isOnGround())
                return ExecutionStatus::Running;
            // Mining already inspected every breakable swept cell above. A
            // one-tick world update, exposed liquid/hazard, or stale chunk
            // state must not cause an immediate OffPath/replan after every
            // destroyed block. Holding still lets the existing 80-tick stall
            // detector decide whether a real recovery is necessary.
            if (terrainBreakingAllowed)
                return ExecutionStatus::Running;
            lastFailureReason = "blocked movement clearance";
            return ExecutionStatus::OffPath;
        }
    }

    const bool isBridge = node.movement == MovementType::Bridge;
    const bool isBuildAscend = node.movement == MovementType::BuildAscend;
    if ((isBridge || isBuildAscend) && activeBridgeIndex != index) {
        activeBridgeIndex = index;
        bridgeNextStep = 1;
        bridgePlacementWait = 0;
        bridgePlacementAttempts = 0;
    }
    if (!isBridge && !isBuildAscend) {
        restoreBridgeHotbar(player);
        activeBridgeIndex = static_cast<std::size_t>(-1);
    }
    if (isBuildAscend && index > 0) {
        const auto& source = path[index - 1].pos;
        const int dx = std::clamp(node.pos.x - source.x, -1, 1);
        const int dz = std::clamp(node.pos.z - source.z, -1, 1);
        if (bridgePlacementWait > 0)
            --bridgePlacementWait;
        if (bridgePlacementWait == 0 && bridgeNextStep == 1) {
            // BuildAscend's raised destination is supported by the block we
            // place directly ahead at the source level. Place it from the
            // source side, then let the normal one-block jump input take over.
            const BlockPos placementTarget = source.offset(dx, 0, dz);
            const glm::vec2 axis{static_cast<float>(dx), static_cast<float>(dz)};
            const glm::vec2 sourceCenter{source.x + 0.5f, source.z + 0.5f};
            const glm::vec2 playerOffset{feet.x - sourceCenter.x, feet.z - sourceCenter.y};
            const float along = glm::dot(playerOffset, glm::normalize(axis));
            const FacingID supportFace = dx > 0 ? FacingID::West : dx < 0 ? FacingID::East :
                (dz > 0 ? FacingID::North : FacingID::South);
            if (along >= -0.35f) {
                if (bridgePlacementAttempts >= 3) {
                    clearInput(player);
                    lastFailureReason = "build placement retry limit";
                    return ExecutionStatus::OffPath;
                }
                ++bridgePlacementAttempts;
                bridgePlacementWait = 4;
                if (placeBridgeBlock(player, placementTarget, supportFace)) {
                    bridgeNextStep = 2;
                    bridgePlacementAttempts = 0;
                    bridgePlacementWait = 2;
                } else {
                    // A vanilla use action spans several game/update ticks.
                    // Do not keep walking and move the aimed face underneath
                    // the player while Minecraft prepares the transaction.
                    clearInput(player);
                    return ExecutionStatus::Running;
                }
            }
        }
    }
    if (isBridge && index > 0) {
        const auto& source = path[index - 1].pos;
        const int dx = std::clamp(node.pos.x - source.x, -1, 1);
        const int dz = std::clamp(node.pos.z - source.z, -1, 1);
        const int distance = std::max(std::abs(node.pos.x - source.x), std::abs(node.pos.z - source.z));
        const bool diagonalBridge = dx != 0 && dz != 0;
        const int requiredBlocks = diagonalBridge
            ? std::max(1, distance * 2 - 1)
            : std::max(1, distance - 1);
        if (bridgePlacementWait > 0) {
            --bridgePlacementWait;
        }
        if (bridgePlacementWait == 0 && bridgeNextStep <= requiredBlocks) {
            const int step = bridgeNextStep;
            BlockPos placementTarget{};
            int placementDx = dx;
            int placementDz = dz;
            if (diagonalBridge) {
                const int diagonalStep = (step + 1) / 2;
                const bool xStep = (step & 1) != 0;
                placementTarget = xStep
                    ? source.offset(dx * diagonalStep, -1, dz * (diagonalStep - 1))
                    : source.offset(dx * diagonalStep, -1, dz * diagonalStep);
                placementDx = xStep ? dx : 0;
                placementDz = xStep ? 0 : dz;
            } else {
                placementTarget = source.offset(dx * step, -1, dz * step);
            }
            const FacingID supportFace = placementDx > 0 ? FacingID::West : placementDx < 0 ? FacingID::East :
                placementDz > 0 ? FacingID::North : FacingID::South;
            if (bridgeOverWaterOnly && MC::getRegion() != nullptr) {
                const BedrockWorld world(MC::getRegion());
                const auto placementState = world.getBlock(placementTarget);
                // A solid cell means this segment was already placed. Any new
                // construction target must still contain safe water; if the
                // world changed to air or lava, discard and recalculate.
                if (!placementState.loaded || placementState.hazard ||
                    (!placementState.solid && !placementState.liquid)) {
                    clearInput(player);
                    lastFailureReason = "unsafe bridge support";
                    return ExecutionStatus::OffPath;
                }
            }
            glm::vec2 bridgeAxis{static_cast<float>(dx), static_cast<float>(dz)};
            if (glm::length(bridgeAxis) > 0.001f)
                bridgeAxis = glm::normalize(bridgeAxis);
            const glm::vec2 sourceCenter{source.x + 0.5f, source.z + 0.5f};
            const glm::vec2 playerOffset{feet.x - sourceCenter.x, feet.z - sourceCenter.y};
            const float along = glm::dot(playerOffset, bridgeAxis);
            // Do not place the next segment from the middle of the current
            // block. Reach its forward edge first, as a real crouch-bridge
            // player does, then place one block and continue backward.
            const glm::vec2 placementOffset{
                static_cast<float>(placementTarget.x - source.x),
                static_cast<float>(placementTarget.z - source.z)};
            const float projectedPlacement = glm::dot(placementOffset, bridgeAxis);
            const float edgeThreshold = diagonalBridge
                ? std::max(0.32f, projectedPlacement - 0.38f)
                : (step == 1 ? 0.32f : static_cast<float>(step) - 0.38f);
            if (along >= edgeThreshold) {
                if (bridgePlacementAttempts >= 3) {
                    clearInput(player);
                    lastFailureReason = "bridge placement retry limit";
                    return ExecutionStatus::OffPath;
                }
                ++bridgePlacementAttempts;
                bridgePlacementWait = 4;
                if (placeBridgeBlock(player, placementTarget, supportFace)) {
                    bridgeNextStep = step + 1;
                    bridgePlacementAttempts = 0;
                    bridgePlacementWait = 1;
                } else {
                    clearInput(player);
                    return ExecutionStatus::Running;
                }
            }
        }
    }
    const bool isParkour = node.movement == MovementType::Parkour;
    if (isParkour && index > 0 && player->isOnGround() &&
        playerFeetBlock != path[index - 1].pos) {
        // A pre-launch walk-off changes the usable runway. Never keep steering
        // toward the stale upper source (which previously produced a crouched
        // edge loop); replan immediately from the real supported block.
        clearInput(player);
        lastFailureReason = "left parkour source before jumping";
        return ExecutionStatus::OffPath;
    }
    bool narrowFooting = false;
    if (player->isOnGround() && index > 0 && MC::getRegion() != nullptr) {
        const auto& source = path[index - 1].pos;
        const int stepX = std::clamp(node.pos.x - source.x, -1, 1);
        const int stepZ = std::clamp(node.pos.z - source.z, -1, 1);
        if (stepX != 0 || stepZ != 0) {
            const BlockPos playerFeet{
                static_cast<int>(std::floor(feet.x)),
                static_cast<int>(std::round(feet.y)),
                static_cast<int>(std::floor(feet.z))
            };
            const BedrockWorld world(MC::getRegion());
            const bool currentLeft = hasSafeSupport(world, playerFeet.offset(-stepZ, 0, stepX));
            const bool currentRight = hasSafeSupport(world, playerFeet.offset(stepZ, 0, -stepX));
            const bool targetLeft = hasSafeSupport(world, node.pos.offset(-stepZ, 0, stepX));
            const bool targetRight = hasSafeSupport(world, node.pos.offset(stepZ, 0, -stepX));
            // A platform edge still has room on one side and does not need to
            // be slowed. Enable precision movement only when both side supports
            // are absent at the current or destination block: a true one-wide
            // bridge/pillar with meaningful fall risk.
            narrowFooting = (!currentLeft && !currentRight) || (!targetLeft && !targetRight);
        }
    }
    const bool ordinaryMovement = node.movement == MovementType::Traverse ||
        node.movement == MovementType::Diagonal || node.movement == MovementType::BreakTraverse;
    bool upcomingVerticalOrParkour = false;
    bool upcomingTurn = false;
    bool tightObstacleTurn = false;
    if (index > 0 && index + 1 < path.size()) {
        const auto& next = path[index + 1];
        const auto& source = path[index - 1].pos;
        const int currentX = std::clamp(node.pos.x - source.x, -1, 1);
        const int currentZ = std::clamp(node.pos.z - source.z, -1, 1);
        const int nextX = std::clamp(next.pos.x - node.pos.x, -1, 1);
        const int nextZ = std::clamp(next.pos.z - node.pos.z, -1, 1);
        upcomingTurn = currentX != nextX || currentZ != nextZ;
        upcomingVerticalOrParkour = next.movement == MovementType::Ascend ||
            next.movement == MovementType::BreakAscend || next.movement == MovementType::BreakDescend ||
            next.movement == MovementType::BreakDown ||
            next.movement == MovementType::Descend || next.movement == MovementType::Fall ||
            next.movement == MovementType::WaterDrop ||
            next.movement == MovementType::Parkour ||
            next.movement == MovementType::BuildAscend;

        // Smooth look-ahead is useful in open terrain but can swing the
        // player's 0.6-block-wide body into a trunk or wall beside an inside
        // corner. Detect solid columns around both halves of the turn and use
        // exact block-centre steering there.
        if (ordinaryMovement && upcomingTurn && !upcomingVerticalOrParkour &&
            next.pos.y == node.pos.y && MC::getRegion() != nullptr) {
            const BedrockWorld world(MC::getRegion());
            const std::array<BlockPos, 2> turnCenters{{source, node.pos}};
            for (const auto& center : turnCenters) {
                for (int offsetX = -1; offsetX <= 1 && !tightObstacleTurn; ++offsetX) {
                    for (int offsetZ = -1; offsetZ <= 1; ++offsetZ) {
                        if (offsetX == 0 && offsetZ == 0)
                            continue;
                        const auto candidate = center.offset(offsetX, 0, offsetZ);
                        if (candidate == source || candidate == node.pos || candidate == next.pos)
                            continue;
                        const auto feetState = world.getBlock(candidate);
                        const auto headState = world.getBlock(candidate.offset(0, 1, 0));
                        if ((feetState.loaded && feetState.solid) ||
                            (headState.loaded && headState.solid)) {
                            tightObstacleTurn = true;
                            break;
                        }
                    }
                }
                if (tightObstacleTurn)
                    break;
            }
        }
    }
    // On an exposed corner, Baritone-style safe-walk is preferable to trying
    // to compensate after momentum has already carried the player over air.
    float activeEdgeProgress = 0.f;
    if (index > 0)
        activeEdgeProgress = projectOntoSegment(feet, path[index - 1].pos, node.pos).progress;
    // A validated endpoint may be on a treetop or ledge. If no extension
    // exists yet, use safe-walk on the final approach to reduce overshoot.
    const bool endpointApproach = index + 1 == path.size() && ordinaryMovement &&
        activeEdgeProgress >= 0.58f && player->isOnGround();
    const bool approachingBreakDown = index + 1 < path.size() &&
        path[index + 1].movement == MovementType::BreakDown &&
        player->isOnGround() && activeEdgeProgress >= 0.45f;
    const bool precisionSneak = endpointApproach || approachingBreakDown ||
        (narrowFooting && ordinaryMovement && upcomingTurn &&
            !upcomingVerticalOrParkour && activeEdgeProgress >= 0.58f &&
            player->isOnGround());
    if (isParkour) {
        if (activeParkourIndex != index) {
            resetParkourState();
            activeParkourIndex = index;
        }
        if (!player->isOnGround())
            parkourWasAirborne = true;
        if (!player->isOnGround() && !parkourJumpIssued) {
            // We slipped from the runway without issuing a jump. Release every
            // movement key until landing; the grounded source check above will
            // then replan from the actual block instead of adding overshoot.
            clearInput(player);
            return ExecutionStatus::Running;
        }
        // If the launch became airborne but this same edge is still active on
        // the next grounded tick, the destination was missed. Stop immediately
        // and let the controller replan instead of walking off another edge.
        if (parkourJumpIssued && parkourWasAirborne && player->isOnGround()) {
            clearInput(player);
            lastFailureReason = "jump landed outside target";
            return ExecutionStatus::Stuck;
        }
        if (parkourJumpIssued && !parkourWasAirborne && ++parkourLaunchTicks > 10) {
            clearInput(player);
            lastFailureReason = "jump failed to launch";
            return ExecutionStatus::Stuck;
        }
    } else {
        resetParkourState();
    }

    const glm::vec3 target{static_cast<float>(node.pos.x) + 0.5f, static_cast<float>(node.pos.y), static_cast<float>(node.pos.z) + 0.5f};
    glm::vec2 direction{target.x - feet.x, target.z - feet.z};
    glm::vec2 facingDirection = direction;
    glm::vec2 shaftApproachWorldInput{};
    if (approachingBreakDown) {
        const float centerDistance = glm::length(direction);
        if (!bedrock_physics::shaftFootprintCentered(direction.x, direction.y)) {
            const glm::vec2 towardCenter = direction / centerDistance;
            const float inwardSpeed = glm::dot(
                glm::vec2{measuredMotion.x, measuredMotion.z}, towardCenter);
            shaftApproachWorldInput = towardCenter *
                bedrock_physics::shaftCenterInput(centerDistance, inwardSpeed);
        }
    }
    bool ascendTakeoffReady = !isAscending;
    float ascendApproachScale = 1.f;
    if (isAscending && index > 0) {
        const auto& source = path[index - 1].pos;
        glm::vec2 axis{static_cast<float>(node.pos.x - source.x),
            static_cast<float>(node.pos.z - source.z)};
        const float axisLength = glm::length(axis);
        if (axisLength > 0.001f)
            axis /= axisLength;

        const glm::vec2 sourceCenter{static_cast<float>(source.x) + 0.5f,
            static_cast<float>(source.z) + 0.5f};
        const glm::vec2 sourceOffset{feet.x - sourceCenter.x, feet.z - sourceCenter.y};
        const float along = glm::dot(sourceOffset, axis);
        const float lateral = glm::length(sourceOffset - axis * along);
        const float alongSpeed = glm::dot(glm::vec2{measuredMotion.x, measuredMotion.z}, axis);
        const float predictedAlong = along + std::max(0.f, alongSpeed);
        const bool buildReady = !isBuildAscend ||
            (bridgeNextStep > 1 && bridgePlacementWait == 0);
        // Bedrock can report the block as air one or two simulation ticks
        // before the local collision shape fully disappears. Let freshly
        // mined ascent clearance settle before committing the jump.
        const bool terrainReady = !terrainBreakingAllowed || miningRecoveryTicks <= 27;
        // Launch from the forward half of the source block while centered on
        // the movement axis. This leaves enough horizontal travel to clear the
        // step without jumping from the rear edge into the source ceiling.
        ascendTakeoffReady = player->isOnGround() && playerFeetBlock == source &&
            lateral <= 0.24f && along >= 0.04f && predictedAlong >= 0.12f &&
            along <= 0.58f && ascendRetryDelay == 0 && buildReady && terrainReady;
        if (!inWaterBlocks && !ascendJumpIssued && !ascendTakeoffReady) {
            const glm::vec2 takeoffPoint = sourceCenter + axis * 0.18f;
            direction = takeoffPoint - glm::vec2{feet.x, feet.z};
            facingDirection = axis;
            ascendApproachScale = 0.62f;
        }
    }
    if (isBridge && index > 0) {
        const auto& source = path[index - 1].pos;
        glm::vec2 bridgeDirection{static_cast<float>(node.pos.x - source.x), static_cast<float>(node.pos.z - source.z)};
        if (glm::length(bridgeDirection) > 0.001f) {
            bridgeDirection = glm::normalize(bridgeDirection);
            direction = bridgeDirection;
            facingDirection = -bridgeDirection;
            const float eyeY = feet.y + 1.62f;
            const float horizontal = std::max(0.25f, glm::length(glm::vec2{
                static_cast<float>(node.pos.x) + 0.5f - feet.x,
                static_cast<float>(node.pos.z) + 0.5f - feet.z}));
            bridgePitchTarget = std::clamp(std::atan2(eyeY - static_cast<float>(node.pos.y), horizontal) *
                180.f / std::numbers::pi_v<float>, 38.f, 78.f);
        }
    }
    int parkourDistance = 0;
    bool parkourAscend = false;
    bool parkourReady = !isParkour;
    float parkourAlong = 0.f;
    float parkourProgress = 0.f;
    float parkourAlongSpeed = 0.f;
    float parkourLateralCorrection = 0.f;
    if (isParkour && index > 0) {
        const auto& source = path[index - 1].pos;
        const glm::vec2 sourceCenter{static_cast<float>(source.x) + 0.5f, static_cast<float>(source.z) + 0.5f};
        glm::vec2 jumpDirection{static_cast<float>(node.pos.x - source.x), static_cast<float>(node.pos.z - source.z)};
        parkourDistance = std::abs(node.pos.x - source.x) + std::abs(node.pos.z - source.z);
        parkourAscend = node.pos.y > source.y;

        const float jumpLength = glm::length(jumpDirection);
        if (jumpLength > 0.001f) {
            jumpDirection /= jumpLength;
            facingDirection = jumpDirection;
            const glm::vec2 fromSource{feet.x - sourceCenter.x, feet.z - sourceCenter.y};
            parkourAlong = glm::dot(fromSource, jumpDirection);
            parkourProgress = parkourAlong / jumpLength;
            const glm::vec2 lateralOffset = fromSource - jumpDirection * parkourAlong;
            const float lateral = glm::length(lateralOffset);
            parkourAlongSpeed = glm::dot(glm::vec2{measuredMotion.x, measuredMotion.z}, jumpDirection);
            // Limiter action corridor: only launch while still centered on
            // the source block and aimed down the actual parkour segment.
            parkourReady = player->isOnGround() && lateral <= 0.45f &&
                parkourAlong >= -0.65f && parkourAlong <= bedrock_physics::safeTakeoffEdge;
            const bool needsRunway = parkourDistance >= 3 || parkourAscend;
            const bool speedReady = bedrock_physics::parkourSprintReady(
                parkourDistance, parkourAscend, options.sprint,
                parkourSprintTicks, parkourAlongSpeed);
            if (!parkourJumpIssued && player->isOnGround() && needsRunway &&
                !speedReady && parkourAlong >= 0.25f)
                parkourRepositioning = true;
            if (parkourRepositioning) {
                // Reset on the supported source block before another run-up.
                // Use position error plus measured-velocity damping instead
                // of alternating forward/back input around a single point.
                // The latter can oscillate forever on a three-block jump.
                const glm::vec2 runwayStart = sourceCenter - jumpDirection * 0.20f;
                const glm::vec2 runwayError = runwayStart - glm::vec2{feet.x, feet.z};
                const glm::vec2 horizontalMotion{measuredMotion.x, measuredMotion.z};
                direction = runwayError - horizontalMotion * 2.25f;
                const float repositionInputLength = glm::length(direction);
                if (repositionInputLength > 0.60f)
                    direction = direction / repositionInputLength * 0.60f;
                if (glm::length(runwayError) <= 0.14f && lateral <= 0.20f &&
                    std::abs(parkourAlongSpeed) < 0.065f) {
                    parkourRepositioning = false;
                    parkourSprintTicks = 0;
                }
                parkourReady = false;
            }
            if (!parkourRepositioning && (parkourReady || parkourJumpIssued)) {
                // Baritone keeps moving toward the destination throughout the
                // jump. Preserve the route tangent while correcting lateral
                // drift, rather than locking air input to a blind straight W.
                direction = jumpDirection - lateralOffset * (player->isOnGround() ? 0.65f : 1.10f);
                parkourLateralCorrection = lateral;
            } else if (!parkourRepositioning) {
                // PREPPING phase: return to the takeoff corridor before any
                // sprint or jump input is allowed.
                direction = sourceCenter - glm::vec2{feet.x, feet.z};
            }
        }
    }
    bool fallControlActive = false;
    bool cautiousDropSneak = false;
    glm::vec2 fallWorldInput{};
    if ((node.movement == MovementType::Descend || node.movement == MovementType::BreakDescend || node.movement == MovementType::Fall ||
        node.movement == MovementType::WaterDrop) && index > 0 &&
        (!player->isOnGround() || playerFeetBlock == path[index - 1].pos)) {
        const auto& source = path[index - 1].pos;
        // Preserve the edge's forward tangent throughout the drop. Targeting
        // the landing center directly makes the vector reverse after momentum
        // carries the player beyond it.
        direction = {static_cast<float>(node.pos.x - source.x), static_cast<float>(node.pos.z - source.z)};
        facingDirection = direction;

        const glm::vec2 fallVector{static_cast<float>(node.pos.x - source.x),
            static_cast<float>(node.pos.z - source.z)};
        const float fallLength = glm::length(fallVector);
        if (fallLength > 0.001f) {
            const int verticalDrop = std::max(0, source.y - node.pos.y);
            const bool longDrop = verticalDrop >= 2 &&
                node.movement != MovementType::WaterDrop;
            const glm::vec2 fallDirection = fallVector / fallLength;
            const glm::vec2 sourceCenter{source.x + 0.5f, source.z + 0.5f};
            const glm::vec2 fromSource{feet.x - sourceCenter.x, feet.z - sourceCenter.y};
            const float along = glm::dot(fromSource, fallDirection);
            const glm::vec2 lateralOffset = fromSource - fallDirection * along;
            // Keep looking along the route, but use Bedrock air control to pull
            // the body toward the landing centerline for the entire descent.
            direction = fallDirection - lateralOffset * 1.35f;
            facingDirection = fallDirection;
            const auto projection = projectOntoSegment(feet, source, node.pos);
            const float alongSpeed = glm::dot(glm::vec2{measuredMotion.x, measuredMotion.z}, fallDirection);

            const int landingTicks = bedrock_physics::ticksUntilHeight(
                feet.y, measuredMotion.y, target.y, 20);
            const bool cautiousDrop = longDrop && (!player->isOnGround() ||
                bedrock_physics::needsCautiousDropApproach(verticalDrop, false,
                    projection.progress, alongSpeed, fallLength, landingTicks));
            // Solve for the path-axis input that lands just before the block
            // centre. This is camera-independent: applying a brake only to
            // local W left local A/D still accelerating along the route when
            // the camera was turned, which caused moving drops to overshoot.
            // Slow to a reproducible walk-off speed before losing ground
            // control. A sprint-speed takeoff cannot always be recovered by
            // vanilla air acceleration, especially on multi-block drops.
            const float dropEntrySpeed = cautiousDrop ? 0.06f : 0.10f;
            const float landingProgress = cautiousDrop ? 0.80f : 0.90f;
            // Direction flags are digital in the native controller. During a
            // multi-block fall, even a tiny positive analog correction keeps
            // the forward key held and behaves like full air acceleration.
            // Enter slowly, then coast; lateral centering remains available.
            const float alongInput = player->isOnGround()
                ? (cautiousDrop
                    ? bedrock_physics::cautiousDropGroundInput(
                        projection.progress, alongSpeed)
                    : std::max(0.f, bedrock_physics::groundInputForTargetVelocity(
                        alongSpeed, dropEntrySpeed)))
                : bedrock_physics::dropAirInput(verticalDrop,
                    projection.progress, alongSpeed, fallLength, landingTicks,
                    landingProgress);
            // The source approach already centers the player. During a long
            // fall, release all movement keys rather than letting a tiny
            // lateral analog value become a full native strafe direction.
            fallWorldInput = cautiousDrop && !player->isOnGround()
                ? glm::vec2{}
                : fallDirection * alongInput - lateralOffset * 1.35f;
            cautiousDropSneak = cautiousDrop && player->isOnGround() &&
                projection.progress < bedrock_physics::cautiousDropSneakReleaseProgress;
            const float fallInputMagnitude = glm::length(fallWorldInput);
            if (fallInputMagnitude > 1.f)
                fallWorldInput /= fallInputMagnitude;
            fallControlActive = node.movement != MovementType::WaterDrop;
        }
    }
    const float distance = glm::length(direction);
    if (distance > 0.001f)
        direction /= distance;

    // Keep ordinary movement on the exact block-center rail. The old
    // look-ahead tangent is retained below for reference but disabled because
    // it can move the collision box into an inside corner before centering.
    if (false && !terrainBreakingAllowed && ordinaryMovement &&
        !tightObstacleTurn &&
        index > 0 && index + 1 < path.size()) {
        const auto& source = path[index - 1].pos;
        const auto& next = path[index + 1];
        if ((next.movement == MovementType::Traverse || next.movement == MovementType::Diagonal ||
            next.movement == MovementType::BreakTraverse) &&
            next.pos.y == node.pos.y) {
            glm::vec2 currentTangent{static_cast<float>(node.pos.x - source.x),
                static_cast<float>(node.pos.z - source.z)};
            glm::vec2 nextTangent{static_cast<float>(next.pos.x - node.pos.x),
                static_cast<float>(next.pos.z - node.pos.z)};
            const int currentX = std::clamp(node.pos.x - source.x, -1, 1);
            const int currentZ = std::clamp(node.pos.z - source.z, -1, 1);
            const int nextX = std::clamp(next.pos.x - node.pos.x, -1, 1);
            const int nextZ = std::clamp(next.pos.z - node.pos.z, -1, 1);
            bool sweptCornerClear = true;
            if ((currentX != nextX || currentZ != nextZ) && MC::getRegion() != nullptr) {
                const BedrockWorld world(MC::getRegion());
                // Curving before a waypoint sweeps the 0.6-block-wide body
                // through this inside column, which an L-shaped block route
                // does not otherwise occupy.
                sweptCornerClear = hasPlayerClearance(world, source.offset(nextX, 0, nextZ), waterAllowed);
            }
            if (sweptCornerClear && glm::length(currentTangent) > 0.001f &&
                glm::length(nextTangent) > 0.001f) {
                currentTangent = glm::normalize(currentTangent);
                nextTangent = glm::normalize(nextTangent);
                const float segmentProgress = std::clamp(
                    projectOntoSegment(feet, source, node.pos).progress, 0.f, 1.f);
                const float turnAmount = std::clamp((segmentProgress - 0.35f) / 0.65f, 0.f, 1.f) * 0.42f;
                const glm::vec2 tangent = glm::normalize(currentTangent * (1.f - turnAmount) +
                    nextTangent * turnAmount);
                direction = tangent;
                facingDirection = tangent;
            }
        }
    }
    // Human players naturally ease into each waypoint instead of holding a
    // perfect unit input until the exact block boundary. Keep this only for
    // ordinary ground traversal; parkour and falls retain their physics-driven
    // launch/landing inputs.
    const float humanApproachScale = ordinaryMovement
        ? (tightObstacleTurn
            ? std::clamp(distance / 0.95f, 0.48f, 0.74f)
            : (terrainBreakingAllowed && upcomingTurn
            ? std::clamp(distance / 1.00f, 0.50f, 0.75f)
            : std::clamp(distance / 1.15f, 0.82f, 1.f)))
        : 1.f;

    const float facingDistance = glm::length(facingDirection);
    if (facingDistance > 0.001f)
        facingDirection /= facingDistance;

    // Keep path-facing rotation as a render target only. Phase leaves the
    // player's real camera yaw authoritative and converts the world-space path
    // direction into camera-relative forward/strafe input below.
    if (facingDistance > 0.001f) {
        auto rotation = player->getRotation();
        if (isBridge && !bridgePitchActive) {
            savedBridgePitch = rotation.x;
            bridgePitch = rotation.x;
            visualPitch = rotation.x;
            bridgePitchActive = true;
        } else if (!isBridge && bridgePitchActive) {
            rotation.x = savedBridgePitch;
            bridgePitchActive = false;
        }
        const float desiredYaw = std::atan2(-facingDirection.x, facingDirection.y) * 180.f / std::numbers::pi_v<float>;
        cameraYaw = rotation.y;
        cameraYawCaptured = true;
        pathMovementYaw = desiredYaw;
        pathRotationActive = true;
        if (!visualYawInitialized) {
            visualYaw = rotation.y;
            visualBodyYaw = rotation.y;
            visualYawInitialized = true;
        }
    }

    const float yaw = cameraYaw * std::numbers::pi_v<float> / 180.f;
    const glm::vec2 forward{-std::sin(yaw), std::cos(yaw)};
    const glm::vec2 right{forward.y, -forward.x};
    const float parkourLength = static_cast<float>(std::max(parkourDistance, 1));
    const float requestedForward = std::clamp(glm::dot(forward, direction), -1.f, 1.f);
    const float directionLength = glm::length(direction);
    const float intendedPathAlignment = directionLength > 0.001f
        ? glm::dot(direction / directionLength, facingDirection)
        : 0.f;
    const bool parkourNeedsSprint = isParkour && (parkourDistance >= 3 || parkourAscend);

    // Decide before crossing the takeoff point. At full Bedrock sprint speed a
    // player travels about 0.28 blocks per tick, so waiting until the centre is
    // already at a fixed trigger can walk the 0.6-wide body off the source.
    const float measuredApproachSpeed = std::max(parkourAlongSpeed, 0.f);
    const float nextApproachSpeed = bedrock_physics::nextGroundVelocity(
        measuredApproachSpeed, 1.f, parkourNeedsSprint, false);
    const float nextParkourAlong = parkourAlong + nextApproachSpeed;
    const float baseTakeoffDistance = parkourAscend ? 0.34f :
        (parkourDistance >= 4 ? 0.42f : (parkourDistance == 3 ? 0.62f : 0.22f));
    // Faster approaches launch slightly sooner. The small continuous shift is
    // less robotic than separate hard-coded slow/fast movement states.
    const float takeoffDistance = baseTakeoffDistance -
        std::clamp((measuredApproachSpeed - 0.18f) * 0.75f, 0.f, 0.075f);
    const bool sprintReady = bedrock_physics::parkourSprintReady(
        parkourDistance, parkourAscend, options.sprint,
        parkourSprintTicks, measuredApproachSpeed);
    const bool requestParkourJump = isParkour && parkourReady && !parkourJumpIssued && sprintReady &&
        nextParkourAlong >= takeoffDistance && intendedPathAlignment > 0.90f;
    // If sprint has not become valid before the final safe part of the source,
    // release W first. Only use a small reverse input at the last margin; this
    // preserves a natural runway while preventing a momentum-driven walk-off.
    const bool parkourApproachRelease = isParkour && parkourReady && !parkourJumpIssued &&
        !requestParkourJump && nextParkourAlong >= bedrock_physics::safeTakeoffEdge - 0.08f;
    const bool parkourApproachBrake = parkourApproachRelease &&
        parkourAlong >= bedrock_physics::safeTakeoffEdge - 0.14f;

    // Predict the landing from the SDK's measured velocity using Bedrock's
    // gravity, drag, and air acceleration. Releasing W handles a mild projected
    // overshoot; reverse air input is reserved for a clearly missed landing.
    float predictedLandingWithForward = parkourAlong;
    float predictedLandingCoasting = parkourAlong;
    if (isParkour && parkourJumpIssued && !player->isOnGround()) {
        const int landingTicks = bedrock_physics::ticksUntilLandingPlane(
            feet.y, measuredMotion.y, target.y, 30);
        predictedLandingWithForward += bedrock_physics::projectedAirDisplacement(
            measuredApproachSpeed, 1.f, parkourNeedsSprint, false, landingTicks);
        predictedLandingCoasting += bedrock_physics::projectedAirDisplacement(
            measuredApproachSpeed, 0.f, false, false, landingTicks);
    }
    // Coast only when existing momentum can already reach the useful landing
    // area. This avoids rapidly alternating W as the forward-input forecast
    // crosses the target from one tick to the next.
    const bool parkourAirRelease = isParkour && parkourJumpIssued && !player->isOnGround() &&
        parkourProgress > 0.35f && predictedLandingWithForward >= parkourLength - 0.05f &&
        predictedLandingCoasting >= parkourLength - 0.20f;
    const bool parkourAirBrake = parkourAirRelease &&
        predictedLandingCoasting >= parkourLength + 0.25f;

    float cautiousScale = parkourRepositioning ? 0.35f : 1.f;
    if (!isParkour) {
        if (approachingBreakDown)
            cautiousScale = 0.55f;
        else if (precisionSneak)
            cautiousScale = 0.82f;
    }
    const float pathMovementScale = cautiousScale * humanApproachScale * ascendApproachScale;
    const float forwardAmount = parkourAirBrake ? -0.24f :
        (parkourAirRelease ? 0.f : (parkourApproachBrake ? -0.16f :
        (parkourApproachRelease ? 0.f : requestedForward * pathMovementScale)));
    // Keep lateral correction active while braking so an imperfect launch is
    // pulled back over the landing block instead of drifting beside it.
    const float strafeAmount = std::clamp(glm::dot(right, direction), -1.f, 1.f) *
        ((parkourAirRelease || parkourAirBrake) ?
            std::clamp(parkourLateralCorrection * 2.5f, 0.25f, 1.f) : 1.f);
    glm::vec2 localMovement{strafeAmount, forwardAmount};
    if (approachingBreakDown) {
        localMovement = {glm::dot(right, shaftApproachWorldInput),
            glm::dot(forward, shaftApproachWorldInput)};
    }
    if (fallControlActive) {
        localMovement = {glm::dot(right, fallWorldInput), glm::dot(forward, fallWorldInput)};
    }
    if (node.movement == MovementType::WaterDrop && !inWaterBlocks) {
        // Regulate both world axes throughout the fall, including overshoot.
        // Do not normalize this correction or keep a fixed forward tangent.
        glm::vec2 correction{
            bedrock_physics::waterDropAxisInput(target.x - feet.x, measuredMotion.x, player->isOnGround()),
            bedrock_physics::waterDropAxisInput(target.z - feet.z, measuredMotion.z, player->isOnGround())};
        const float magnitude = glm::length(correction);
        if (magnitude > 1.f)
            correction /= magnitude;
        localMovement = {glm::dot(right, correction), glm::dot(forward, correction)};
    }

    // Bridge nodes deliberately retain fb8e96a's input vector byte-for-byte.
    // Placement, crouch movement and the render-only bridge rotation were a
    // working unit in that revision; later ordinary/parkour normalization must
    // not alter the movement state used when buildBlock creates its transaction.
    if (!isBridge) {
        // Vanilla bounds the combined movement stick before acceleration. Keep
        // diagonal path correction inside that same unit circle so a forward +
        // strafe request cannot describe an impossible input to BDS.
        const auto safeMovement = movement_input::sanitize({localMovement.x, localMovement.y});
        localMovement = {safeMovement.x, safeMovement.y};
    }

    bool shouldSprint = false;
    if (const auto input = player->tryGet<MoveInputComponent>()) {
        input->move = localMovement;
        input->inputState.analogMoveVector = localMovement;
        input->rawInputState.analogMoveVector = localMovement;

        const auto directions = movement_input::directions({localMovement.x, localMovement.y});
        input->inputState.up = directions.up;
        input->inputState.down = directions.down;
        input->inputState.left = directions.left;
        input->inputState.right = directions.right;
        input->rawInputState.up = input->inputState.up;
        input->rawInputState.down = input->inputState.down;
        input->rawInputState.left = input->inputState.left;
        input->rawInputState.right = input->inputState.right;
        input->inputState.upLeft = input->rawInputState.upLeft = input->inputState.up && input->inputState.left;
        input->inputState.upRight = input->rawInputState.upRight = input->inputState.up && input->inputState.right;
        input->inputState.downLeft = input->rawInputState.downLeft = input->inputState.down && input->inputState.left;
        input->inputState.downRight = input->rawInputState.downRight = input->inputState.down && input->inputState.right;

        // Java Baritone only forces sprint for the maximum-distance or ascending
        // parkour variants. Sprinting on short gaps causes Bedrock to overshoot.
        // Bedrock may internally limit the final speed while crouched, but keep
        // sprint requested during safe-walk as configured instead of forcibly
        // clearing it. Wide ordinary paths run at full vanilla speed.
        const bool sprintSafe = isParkour ||
            (ordinaryMovement && !upcomingVerticalOrParkour && (!narrowFooting || precisionSneak) &&
                !tightObstacleTurn && (!terrainBreakingAllowed || !upcomingTurn));
        const float sprintThreshold = precisionSneak ? 0.45f : (isParkour ? 0.55f : 0.8f);
        shouldSprint = options.sprint && !parkourRepositioning &&
            forwardAmount > sprintThreshold &&
            (!isParkour || parkourNeedsSprint) && !parkourAirRelease &&
            sprintSafe && !player->isInWater();
        input->inputState.sprintDown = shouldSprint;
        input->rawInputState.sprintDown = shouldSprint;
        input->sprinting = shouldSprint;
        if (isBridge) {
            input->sneaking = true;
            input->persistSneak = true;
            input->inputState.sneakDown = true;
            input->rawInputState.sneakDown = true;
        }

        // Let Bedrock's normal movement systems consume the inputs. Locking
        // this component and writing velocity directly causes server lagbacks.
        input->moveInputStateLocked = false;

        if (isParkour && parkourReady && parkourNeedsSprint && shouldSprint) {
            ++parkourSprintTicks;
        } else if (!parkourJumpIssued) {
            parkourSprintTicks = 0;
        }
    }

    const bool shouldJump = (isAscending && !ascendJumpIssued && ascendTakeoffReady &&
        target.y > feet.y + 0.2f) ||
        requestParkourJump;
    const bool shouldSwimUp = inWaterBlocks && (node.movement == MovementType::Swim || target.y >= feet.y - 0.15f);

    // Pulse the same input edges generated by a real Space press. Do not call
    // jumpFromGround or write velocity: Bedrock owns the jump and its packet
    // prediction, which keeps the result valid for BDS movement checks.
    const bool groundedAtLaunch = player->isOnGround();
    const bool holdJump = (shouldJump && groundedAtLaunch) || shouldSwimUp;
    // Latch the requested launch before the native movement tick consumes it.
    if (holdJump && isAscending && groundedAtLaunch) {
        ascendJumpIssued = true;
        ascendWasAirborne = false;
        ascendLaunchTicks = 0;
    }
    if (requestParkourJump) {
        parkourJumpIssued = true;
        parkourLaunchTicks = 0;
    }
    if (const auto input = player->tryGet<MoveInputComponent>()) {
        const bool jumpWasDown = input->rawInputState.jumpInputCurrentlyDown;
        input->jumping = holdJump;
        input->inputState.jumpDown = holdJump;
        input->inputState.jumpInputCurrentlyDown = holdJump;
        input->rawInputState.jumpDown = holdJump;
        input->rawInputState.jumpInputCurrentlyDown = holdJump;
        if (!isBridge) {
            input->inputState.jumpInputWasPressed = holdJump && !jumpWasDown;
            input->inputState.jumpInputWasReleased = !holdJump && jumpWasDown;
            input->rawInputState.jumpInputWasPressed = holdJump && !jumpWasDown;
            input->rawInputState.jumpInputWasReleased = !holdJump && jumpWasDown;
        }
    }

    if (const auto input = player->tryGet<MoveInputComponent>()) {
        // Keep swimming buoyant; precision ground steering must not cause a dive.
        const bool shouldSneak = isBridge || cautiousDropSneak ||
            ((precisionSneak || parkourRepositioning) && !inWaterBlocks);
        const bool sneakWasDown = input->rawInputState.sneakInputCurrentlyDown;
        input->sneaking = shouldSneak;
        input->wantDown = false;
        input->inputState.sneakDown = shouldSneak;
        input->rawInputState.sneakDown = shouldSneak;
        input->inputState.sneakInputCurrentlyDown = shouldSneak;
        input->rawInputState.sneakInputCurrentlyDown = shouldSneak;
        input->inputState.sneakInputWasPressed =
            input->rawInputState.sneakInputWasPressed = shouldSneak && !sneakWasDown;
        input->inputState.sneakInputWasReleased =
            input->rawInputState.sneakInputWasReleased = !shouldSneak && sneakWasDown;
        captureInputCommand(input, !isBridge);
    }

    if (isParkour) {
        const auto rotation = player->getRotation();
        logF("[Parkour] node={}/{} src=({}, {}, {}) dst=({}, {}, {}) feet=({:.4f},{:.4f},{:.4f}) motion=({:.4f},{:.4f},{:.4f}) ground={} dist={} ascend={} along={:.4f} progress={:.4f} lateral={:.4f} alongSpeed={:.4f} ready={} reposition={} sprintTicks={} sprintReady={} jumpIssued={} airborne={} launchTicks={} nextAlong={:.4f} takeoff={:.4f} align={:.4f} requestJump={} holdJump={} sprint={} release={} brake={} predictedW={:.4f} predictedCoast={:.4f} input=({:.4f},{:.4f}) camera=({:.2f},{:.2f})",
            index, path.size(), path[index - 1].pos.x, path[index - 1].pos.y,
            path[index - 1].pos.z, node.pos.x, node.pos.y, node.pos.z,
            feet.x, feet.y, feet.z, measuredMotion.x, measuredMotion.y, measuredMotion.z,
            player->isOnGround(), parkourDistance, parkourAscend, parkourAlong,
            parkourProgress, parkourLateralCorrection, parkourAlongSpeed, parkourReady,
            parkourRepositioning, parkourSprintTicks, sprintReady, parkourJumpIssued,
            parkourWasAirborne, parkourLaunchTicks, nextParkourAlong, takeoffDistance,
            intendedPathAlignment, requestParkourJump, holdJump, shouldSprint,
            parkourAirRelease, parkourAirBrake, predictedLandingWithForward,
            predictedLandingCoasting, localMovement.x, localMovement.y,
            rotation.x, rotation.y);
    }

    controlledMovement = true;
    return ExecutionStatus::Running;
}

void PathExecutor::captureInputCommand(const MoveInputComponent* input, const bool includeJumpEdges) {
    if (input == nullptr) {
        inputCommandValid = false;
        return;
    }
    inputCommandSnapshot = *input;
    inputCommandIncludesJumpEdges = includeJumpEdges;
    inputCommandValid = true;
}

void PathExecutor::reapplyInput(LocalPlayer* player) {
    if (player == nullptr || !controlledMovement || !inputCommandValid)
        return;

    auto* input = player->tryGet<MoveInputComponent>();
    if (input == nullptr)
        return;

    const auto copyState = [this](MoveInputState& destination, const MoveInputState& source) {
        destination.analogMoveVector = source.analogMoveVector;
        destination.up = source.up;
        destination.down = source.down;
        destination.left = source.left;
        destination.right = source.right;
        destination.upLeft = source.upLeft;
        destination.upRight = source.upRight;
        destination.downLeft = source.downLeft;
        destination.downRight = source.downRight;
        destination.sprintDown = source.sprintDown;
        destination.jumpDown = source.jumpDown;
        destination.jumpInputCurrentlyDown = source.jumpInputCurrentlyDown;
        if (inputCommandIncludesJumpEdges) {
            destination.jumpInputWasPressed = source.jumpInputWasPressed;
            destination.jumpInputWasReleased = source.jumpInputWasReleased;
        }
        destination.sneakDown = source.sneakDown;
        destination.sneakInputCurrentlyDown = source.sneakInputCurrentlyDown;
    };

    input->move = inputCommandSnapshot.move;
    copyState(input->inputState, inputCommandSnapshot.inputState);
    copyState(input->rawInputState, inputCommandSnapshot.rawInputState);
    input->sprinting = inputCommandSnapshot.sprinting;
    input->jumping = inputCommandSnapshot.jumping;
    input->sneaking = inputCommandSnapshot.sneaking;
    input->persistSneak = inputCommandSnapshot.persistSneak;
    input->wantDown = inputCommandSnapshot.wantDown;
    input->moveInputStateLocked = false;
    input->isCameraRelativeMovementEnabled = inputCommandSnapshot.isCameraRelativeMovementEnabled;
    input->isRotControlledByMoveDirection = inputCommandSnapshot.isRotControlledByMoveDirection;
}

void PathExecutor::stop(LocalPlayer* player) {
    clearInput(player);
    if (activeBreakIndex != static_cast<std::size_t>(-1) && player != nullptr &&
        player->getGameMode() != nullptr) {
        const glm::ivec3 target{activeBreakPos.x, activeBreakPos.y, activeBreakPos.z};
        bedrock_block_breaking::stop(player, target);
    }
    restoreMiningHotbar(player);
    path.clear();
    index = 0;
    ticksWithoutProgress = 0;
    ticksOutsidePath = 0;
    progressInitialized = false;
    motionSampleInitialized = false;
    pathRotationActive = false;
    visualYawInitialized = false;
    cameraYawCaptured = false;
    renderRotationOverrideActive = false;
    lastRotationRenderMillis = 0;
    resetParkourState();
    bridgeNextStep = 1;
    bridgePlacementWait = 0;
    bridgePlacementAttempts = 0;
    activeBridgeIndex = static_cast<std::size_t>(-1);
    restoreBridgeHotbar(player);
    bridgePitchActive = false;
    bridgePitch = 0.f;
    activeBreakIndex = static_cast<std::size_t>(-1);
    obstructionBreakTicks = 0;
    terrainBreakingAllowed = false;
    bridgeOverWaterOnly = false;
    miningRecoveryTicks = 0;
    blockedTerrainTicks = 0;
    activeAscendIndex = static_cast<std::size_t>(-1);
    ascendJumpIssued = false;
    ascendWasAirborne = false;
    ascendLaunchTicks = 0;
    ascendRetries = 0;
    ascendRetryDelay = 0;
}

bool PathExecutor::placeBridgeBlock(LocalPlayer* player, const BlockPos& target, const FacingID preferredFace) {
    auto* source = MC::getRegion();
    if (player == nullptr || source == nullptr) return false;
    const glm::ivec3 pos{target.x, target.y, target.z};
    auto* existing = source->getBlock(pos);
    if (existing != nullptr && existing->getBlockLegacy() != nullptr && existing->getBlockLegacy()->isSolid()) return true;
    auto* supplies = player->getSupplies();
    if (supplies == nullptr || supplies->getInventory() == nullptr) return false;
    int blockSlot = -1;
    for (int slot = 0; slot < 9; ++slot) {
        auto* stack = supplies->getInventory()->getItem(slot);
        if (stack != nullptr && stack->isValid() && stack->getItem() != nullptr && stack->isBlockType()) {
            blockSlot = slot;
            break;
        }
    }
    if (blockSlot < 0 || player->getGameMode() == nullptr) return false;

    const int previousSlot = supplies->getSelectedHotbarSlot();
    supplies->setSelectedHotbarSlot(blockSlot);
    const auto restoreSlot = [&]() {
        supplies->setSelectedHotbarSlot(previousSlot);
    };

    static constexpr glm::ivec3 supports[] = {{0,-1,0},{0,1,0},{0,0,-1},{0,0,1},{-1,0,0},{1,0,0}};
    int selectedFace = -1;
    const int preferredIndex = static_cast<int>(preferredFace);
    if (preferredIndex >= 0 && preferredIndex < 6) {
        const auto support = pos + supports[preferredIndex];
        auto* block = source->getBlock(support);
        if (block != nullptr && block->getBlockLegacy() != nullptr &&
            block->getBlockLegacy()->isSolid())
            selectedFace = preferredIndex;
    }
    for (int face = 0; face < 6 && selectedFace < 0; ++face) {
        const auto support = pos + supports[face];
        auto* block = source->getBlock(support);
        if (block != nullptr && block->getBlockLegacy() != nullptr &&
            block->getBlockLegacy()->isSolid())
            selectedFace = face;
    }
    bool placed = false;
    if (selectedFace >= 0) {
        auto place = pos;
        // Exactly one native use action per retry. buildBlock can send a
        // transaction even when it returns false, so probing every face in a
        // single tick creates a packet burst and can disconnect from BDS.
        placed = player->getGameMode()->buildBlock(
            place, static_cast<FacingID>(selectedFace), false);
    }
    restoreSlot();
    return placed;
}

void PathExecutor::restoreBridgeHotbar(LocalPlayer* player) {
    (void)player;
    bridgeHotbarSlot = -1;
    previousBridgeHotbarSlot = -1;
    bridgeEquipWait = 0;
    bridgeAimFace = -1;
    bridgeAimWait = 0;
    bridgeUsePending = false;
    bridgeUseWait = 0;
}

void PathExecutor::selectBestTool(LocalPlayer* player, const BlockPos& target) {
    // Preserve the server-synchronized equipped slot; see MiningProcess.
    (void)player;
    (void)target;
}

void PathExecutor::restoreMiningHotbar(LocalPlayer* player) {
    (void)player;
    previousMiningHotbarSlot = -1;
}

void PathExecutor::suspend(LocalPlayer* player) {
    clearInput(player);
    motionSampleInitialized = false;
    pathRotationActive = false;
    visualYawInitialized = false;
}

void PathExecutor::applyVisualRotation(LocalPlayer* player) {
    // Steering is now converted into camera-relative forward/strafe input and
    // never changes the actor's real yaw. The path-facing rotation is applied
    // only inside the render callbacks, so there is nothing to restore here.
    (void)player;
}

void PathExecutor::beginVisualRotationRender(LocalPlayer* player) {
    if (player == nullptr || !pathRotationActive || !visualYawInitialized || renderRotationOverrideActive)
        return;

    const auto now = TimeUtils::currentTimeMillis();
    const float delta = lastRotationRenderMillis == 0
        ? 1.f / 60.f
        : std::clamp(static_cast<float>(now - lastRotationRenderMillis) / 1000.f, 0.f, 0.1f);
    lastRotationRenderMillis = now;

    while (visualYaw < pathMovementYaw - 180.f)
        visualYaw += 360.f;
    while (visualYaw > pathMovementYaw + 180.f)
        visualYaw -= 360.f;
    visualYaw += (pathMovementYaw - visualYaw) * std::clamp(delta * 2.07f * rotationSmoothness, 0.f, 1.f);

    while (visualBodyYaw < visualYaw - 180.f)
        visualBodyYaw += 360.f;
    while (visualBodyYaw > visualYaw + 180.f)
        visualBodyYaw -= 360.f;
    visualBodyYaw += (visualYaw - visualBodyYaw) * std::clamp(delta * 1.04f * rotationSmoothness, 0.f, 1.f);

    if (auto* head = player->tryGet<ActorHeadRotationComponent>()) {
        savedHeadRotation = head->rotation;
        head->rotation.x = visualYaw;
        head->rotation.y = visualYaw;
    }
    savedActorRotation = player->getRotation();
    // Keep the camera's real pitch. A second render-only bridge pitch caused
    // third person to alternate between looking down and looking forward.
    visualPitch = savedActorRotation.x;
    if (auto* body = player->tryGet<MobBodyRotationComponent>()) {
        savedBodyRotation = body->bodyRotation;
        savedPreviousBodyRotation = body->previousBodyRotation;
        body->bodyRotation = visualBodyYaw;
        body->previousBodyRotation = visualBodyYaw;
    }
    renderRotationOverrideActive = true;
}

void PathExecutor::endVisualRotationRender(LocalPlayer* player) {
    if (!renderRotationOverrideActive)
        return;
    if (player == nullptr) {
        // A world/dimension can disappear between the before/after render
        // callbacks. Never carry the old actor override into the next world.
        renderRotationOverrideActive = false;
        return;
    }
    player->setRotation(savedActorRotation);
    if (auto* head = player->tryGet<ActorHeadRotationComponent>())
        head->rotation = savedHeadRotation;
    if (auto* body = player->tryGet<MobBodyRotationComponent>()) {
        body->bodyRotation = savedBodyRotation;
        body->previousBodyRotation = savedPreviousBodyRotation;
    }
    renderRotationOverrideActive = false;
}

void PathExecutor::clearInput(LocalPlayer* player) {
    if (!controlledMovement)
        return;

    inputCommandValid = false;
    if (player == nullptr) {
        controlledMovement = false;
        return;
    }

    if (bridgePitchActive) {
        auto rotation = player->getRotation();
        rotation.x = savedBridgePitch;
        player->setRotation(rotation);
    bridgePitchActive = false;
    bridgePitchTarget = 62.f;
}

    if (const auto input = player->tryGet<MoveInputComponent>()) {
        input->move = {};
        input->inputState.analogMoveVector = {};
        input->rawInputState.analogMoveVector = {};
        input->inputState.up = false;
        input->inputState.down = false;
        input->inputState.left = false;
        input->inputState.right = false;
        input->inputState.sprintDown = false;
        input->inputState.jumpDown = false;
        input->inputState.jumpInputCurrentlyDown = false;
        input->inputState.jumpInputWasPressed = false;
        input->inputState.jumpInputWasReleased = false;
        input->rawInputState.up = false;
        input->rawInputState.down = false;
        input->rawInputState.left = false;
        input->rawInputState.right = false;
        input->inputState.upLeft = false;
        input->inputState.upRight = false;
        input->inputState.downLeft = false;
        input->inputState.downRight = false;
        input->rawInputState.upLeft = false;
        input->rawInputState.upRight = false;
        input->rawInputState.downLeft = false;
        input->rawInputState.downRight = false;
        input->rawInputState.sprintDown = false;
        input->rawInputState.jumpDown = false;
        input->rawInputState.jumpInputCurrentlyDown = false;
        input->rawInputState.jumpInputWasPressed = false;
        input->rawInputState.jumpInputWasReleased = false;
        input->inputState.sneakDown = false;
        input->rawInputState.sneakDown = false;
        input->inputState.sneakInputCurrentlyDown = false;
        input->rawInputState.sneakInputCurrentlyDown = false;
        input->sprinting = false;
        input->sneaking = false;
        input->persistSneak = false;
        input->jumping = false;
        input->sneaking = false;
        input->wantDown = false;
        input->moveInputStateLocked = false;
    }

    controlledMovement = false;
}

void PathExecutor::resetParkourState() {
    activeParkourIndex = static_cast<std::size_t>(-1);
    parkourJumpIssued = false;
    parkourWasAirborne = false;
    parkourRepositioning = false;
    parkourSprintTicks = 0;
    parkourLaunchTicks = 0;
}

std::size_t PathExecutor::getCurrentIndex() const { return index; }

const std::vector<PathNode>& PathExecutor::getPath() const { return path; }

double PathExecutor::getEstimatedTicksRemaining() const {
    double ticks = 0.0;
    for (std::size_t cursor = index; cursor < path.size(); ++cursor)
        ticks += path[cursor].costFromPrevious;
    return ticks;
}

} // namespace baritone
