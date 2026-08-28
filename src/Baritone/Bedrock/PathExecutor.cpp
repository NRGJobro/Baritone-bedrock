#include "PathExecutor.h"

#include "BedrockPhysics.h"
#include "BedrockWorld.h"
#include "../../SDK/MC.h"
#include "../../SDK/Client/Input/MoveInputComponent.h"
#include "../../SDK/World/Actor/LocalPlayer.h"
#include "../../SDK/World/BlockSource.h"
#include "../../SDK/World/Inventory/Inventory.h"
#include "../../SDK/World/Inventory/PlayerInventory.h"
#include "../../SDK/World/Item/ItemStack.h"
#include "../../SDK/World/Item/Item.h"
#include "../../Utils/TimeUtils.h"

#include <numbers>

namespace baritone {
namespace {

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

bool liquidAt(BlockSource* source, const glm::ivec3& pos) {
    if (source == nullptr) return false;
    auto* block = source->getBlock(pos);
    auto* legacy = block != nullptr ? block->getBlockLegacy() : nullptr;
    auto* material = legacy != nullptr ? legacy->getMaterial() : nullptr;
    return material != nullptr && material->liquid;
}

} // namespace

void PathExecutor::begin(std::vector<PathNode> newPath) {
    path = std::move(newPath);
    index = path.size() > 1 ? 1 : path.size();
    lastProgressPosition = {};
    ticksWithoutProgress = 0;
    ticksOutsidePath = 0;
    progressInitialized = false;
    controlledMovement = false;
    pathRotationActive = false;
    visualYawInitialized = false;
    cameraYawCaptured = false;
    renderRotationOverrideActive = false;
    lastRotationRenderMillis = 0;
    waterDescentActive = false;
    waterBottomKnown = false;
    waterExitIndex = static_cast<std::size_t>(-1);
    waterDescentTicks = 0;
    waterVerticalSettledTicks = 0;
    waterHasDescended = false;
    resetParkourState();
}

ExecutionStatus PathExecutor::tick(LocalPlayer* player, const ExecutionOptions& options) {
    if (player == nullptr)
        return ExecutionStatus::NoPlayer;

    rotationSmoothness = std::clamp(options.rotationSmoothness, 0.25f, 10.f);

    if (index >= path.size()) {
        clearInput(player);
        return ExecutionStatus::Arrived;
    }

    const auto feet = player->getFeetPosition();
    glm::vec3 measuredMotion{};
    if (const auto state = player->tryGet<StateVectorComponent>()) {
        measuredMotion = state->pos - state->posPrev;
        // Ignore teleports/corrections. For ordinary Bedrock movement the
        // observed horizontal displacement is substantially below one block
        // per simulation tick.
        if (!std::isfinite(measuredMotion.x) || !std::isfinite(measuredMotion.y) ||
            !std::isfinite(measuredMotion.z) ||
            glm::length(glm::vec2{measuredMotion.x, measuredMotion.z}) > 1.25f)
            measuredMotion = {};
    }
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

    // Recovery for a fast/accidental descent: the physical player may reach
    // the bottom before the executor has activated or consumed the individual
    // vertical swim nodes. Find that pending column in the route and splice
    // directly to its first horizontal exit instead of steering back upward.
    {
        const std::size_t recoveryEnd = std::min(path.size(), index + 96);
        for (std::size_t candidate = index; candidate < recoveryEnd; ++candidate) {
            if (candidate == 0) continue;
            const auto& previous = path[candidate - 1].pos;
            const auto& current = path[candidate];
            if (current.movement != MovementType::Swim ||
                current.pos.x != previous.x || current.pos.z != previous.z ||
                current.pos.y >= previous.y)
                continue;

            const int columnX = current.pos.x;
            const int columnZ = current.pos.z;
            std::size_t exitIndex = candidate;
            int pathBottomY = current.pos.y;
            while (exitIndex < path.size() &&
                path[exitIndex].movement == MovementType::Swim &&
                path[exitIndex].pos.x == columnX && path[exitIndex].pos.z == columnZ) {
                pathBottomY = std::min(pathBottomY, path[exitIndex].pos.y);
                ++exitIndex;
            }

            const float dx = feet.x - (static_cast<float>(columnX) + 0.5f);
            const float dz = feet.z - (static_cast<float>(columnZ) + 0.5f);
            const bool nearColumn = dx * dx + dz * dz <= 1.35f * 1.35f;
            const bool alreadyAtBottom = feet.y <= static_cast<float>(pathBottomY) + 1.75f;
            bool supportedAtBottom = false;
            if (MC::getRegion() != nullptr) {
                const BedrockWorld world(MC::getRegion());
                supportedAtBottom = hasSafeSupport(world, playerFeetBlock);
            }
            const bool physicallyDescendingOrWet = inWaterBlocks ||
                measuredMotion.y < -0.025f || supportedAtBottom;
            if (nearColumn && alreadyAtBottom && physicallyDescendingOrWet && exitIndex > index) {
                index = exitIndex;
                lastProgressPosition = feet;
                ticksWithoutProgress = 0;
                ticksOutsidePath = 0;
                waterDescentActive = false;
                waterBottomKnown = false;
                waterExitIndex = static_cast<std::size_t>(-1);
                waterDescentTicks = 0;
                waterVerticalSettledTicks = 0;
                waterHasDescended = false;
                resetParkourState();
                if (index >= path.size()) {
                    clearInput(player);
                    return ExecutionStatus::Arrived;
                }
                break;
            }
        }
    }

    // Enter an explicit downward-column state as soon as the active path is
    // about to descend in water. This state survives node/Y mismatches caused
    // by Bedrock's faster crouched descent.
    if (!waterDescentActive) {
        const std::size_t scanEnd = std::min(path.size() - 1, index + 5);
        for (std::size_t candidate = index; candidate <= scanEnd; ++candidate) {
            if (candidate == 0) continue;
            const auto& previous = path[candidate - 1].pos;
            const auto& current = path[candidate];
            if (current.movement == MovementType::Swim &&
                current.pos.x == previous.x && current.pos.z == previous.z &&
                current.pos.y < previous.y &&
                horizontalDistance(feet, glm::vec3{
                    static_cast<float>(current.pos.x) + 0.5f,
                    feet.y,
                    static_cast<float>(current.pos.z) + 0.5f}) <= 2.75f) {
                waterDescentActive = true;
                waterColumnX = current.pos.x;
                waterColumnZ = current.pos.z;
                waterBottomKnown = false;
                waterDescentTicks = 0;
                waterVerticalSettledTicks = 0;
                waterHasDescended = false;
                lastWaterFeetY = feet.y;
                waterFacingYaw = player->getRotation().y;

                waterExitIndex = candidate;
                while (waterExitIndex < path.size() &&
                    path[waterExitIndex].movement == MovementType::Swim &&
                    path[waterExitIndex].pos.x == waterColumnX &&
                    path[waterExitIndex].pos.z == waterColumnZ)
                    ++waterExitIndex;
                waterPathBottomY = waterExitIndex > candidate
                    ? path[waterExitIndex - 1].pos.y
                    : current.pos.y;

                // Read the actual loaded column to find the floor beneath the
                // water. This is independent of actor flags and path-node Y
                // bookkeeping, both of which can lag during fast descent.
                if (MC::getRegion() != nullptr) {
                    const BedrockWorld world(MC::getRegion());
                    bool sawLiquid = false;
                    for (int y = current.pos.y; y >= current.pos.y - 96; --y) {
                        const glm::ivec3 blockPos{waterColumnX, y, waterColumnZ};
                        if (liquidAt(MC::getRegion(), blockPos)) {
                            sawLiquid = true;
                            continue;
                        }
                        if (sawLiquid) {
                            const auto floor = world.getBlock(BlockPos{waterColumnX, y, waterColumnZ});
                            if (floor.loaded && floor.solid) {
                                waterBottomY = y + 1;
                                waterBottomKnown = true;
                            }
                            break;
                        }
                    }
                }
                break;
            }
        }
    }

    // At the physical floor, jump past every unconsumed node belonging to the
    // vertical column and immediately hand control to the first exit node.
    // This deliberately does not depend on isOnGround(), which remains false
    // while Bedrock considers the player's body submerged.
    if (waterDescentActive) {
        ++waterDescentTicks;
        if (feet.y < lastWaterFeetY - 0.025f) {
            waterHasDescended = true;
            waterVerticalSettledTicks = 0;
        } else if (waterHasDescended) {
            ++waterVerticalSettledTicks;
        }
        lastWaterFeetY = feet.y;

        const float columnDx = feet.x - (static_cast<float>(waterColumnX) + 0.5f);
        const float columnDz = feet.z - (static_cast<float>(waterColumnZ) + 0.5f);
        const bool insideColumn = columnDx * columnDx + columnDz * columnDz <= 1.20f * 1.20f;
        const bool reachedScannedFloor = waterBottomKnown && insideColumn &&
            feet.y <= static_cast<float>(waterBottomY) + 0.80f;
        // Motion fallback: at the bottom, crouch remains requested but the
        // player's Y stops decreasing. Requiring a real descent first and the
        // lowest planned column level prevents this from firing at entry.
        const bool settledAtPathBottom = insideColumn && waterHasDescended &&
            waterDescentTicks >= 6 && waterVerticalSettledTicks >= 4 &&
            feet.y <= static_cast<float>(waterPathBottomY) + 1.75f;
        if (reachedScannedFloor || settledAtPathBottom) {
            if (waterExitIndex > index && waterExitIndex <= path.size()) {
                index = waterExitIndex;
                lastProgressPosition = feet;
                ticksWithoutProgress = 0;
                ticksOutsidePath = 0;
                resetParkourState();
            }
            waterDescentActive = false;
            waterBottomKnown = false;
            waterExitIndex = static_cast<std::size_t>(-1);
            waterDescentTicks = 0;
            waterVerticalSettledTicks = 0;
            waterHasDescended = false;
            if (index >= path.size()) {
                clearInput(player);
                return ExecutionStatus::Arrived;
            }
        }
    }
    if (!progressInitialized) {
        lastProgressPosition = feet;
        progressInitialized = true;
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
        const float corridorWidth = path[index].movement == MovementType::Swim ? 1.05f : 0.60f;
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
            if (index > 0) {
                const auto& previous = path[index - 1].pos;
                const bool descendingColumnNode = node.pos.y < previous.y &&
                    node.pos.x == previous.x && node.pos.z == previous.z;
                // A downward swim node is complete once the physical player
                // has crossed its Y plane. Requiring proximity to that exact
                // Y makes already-passed nodes pull the player upward/around
                // the column at the bottom. This loop can now consume every
                // vertical node the player passed during a fast descent.
                if (descendingColumnNode &&
                    dx * dx + dz * dz <= 1.05f * 1.05f &&
                    feet.y <= static_cast<float>(node.pos.y) + 1.10f)
                    reached = true;
            }
            // At a stream exit Bedrock can keep the player half a block above
            // or below the rounded feet cell. Consume only the active water
            // node when tightly centered and vertically aligned.
            if (!reached)
                reached = dx * dx + dz * dz <= 0.62f * 0.62f &&
                    std::abs(feet.y - static_cast<float>(node.pos.y)) <= 1.05f;
            // Bedrock often leaves the liquid volume one tick before the
            // planner's swim node is consumed.  When the player is grounded
            // in the stream's exit corridor, accept that physical landing so
            // the executor does not spin and replan at the bottom.
            if (!reached && player->isOnGround() && MC::getRegion() != nullptr) {
                const BedrockWorld world(MC::getRegion());
                reached = hasSafeSupport(world, playerFeetBlock) &&
                    dx * dx + dz * dz <= 0.90f * 0.90f &&
                    std::abs(feet.y - static_cast<float>(node.pos.y)) <= 1.35f;
            }
            // A column's final swim node can be one block behind the actual
            // standing cell because Bedrock keeps the body submerged during
            // the exit tick.  If the next path node is ground movement and
            // the player has real support, consume the swim tail instead of
            // repeatedly steering back into the column.
            if (!reached && index + 1 < path.size() &&
                path[index + 1].movement != MovementType::Swim &&
                MC::getRegion() != nullptr) {
                const auto& exit = path[index + 1].pos;
                const float ex = feet.x - (static_cast<float>(exit.x) + 0.5f);
                const float ez = feet.z - (static_cast<float>(exit.z) + 0.5f);
                const BedrockWorld world(MC::getRegion());
                reached = hasSafeSupport(world, playerFeetBlock) &&
                    ex * ex + ez * ez <= 1.05f * 1.05f &&
                    std::abs(feet.y - static_cast<float>(exit.y)) <= 1.50f;
            }
        }

        // A Bedrock fall can land past the destination block centre even after
        // forward input is released. Complete the fall from the supported
        // physical landing corridor so the next movement takes control; never
        // steer backward toward the top of the drop.
        if (!reached && index > 0 && player->isOnGround() && MC::getRegion() != nullptr &&
            (node.movement == MovementType::Descend || node.movement == MovementType::Fall) &&
            std::abs(feet.y - static_cast<float>(node.pos.y)) <= 0.70f) {
            const auto landing = projectOntoSegment(feet, path[index - 1].pos, node.pos);
            const BedrockWorld world(MC::getRegion());
            reached = hasSafeSupport(world, playerFeetBlock) &&
                landing.progress >= 0.60f && landing.progress <= 2.20f &&
                landing.lateralDistance <= 0.55f;
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

        if (!reached)
            break;
        ++index;
    }

    if (index >= path.size()) {
        clearInput(player);
        return ExecutionStatus::Arrived;
    }

    // Do not teleport progress to a merely nearby future segment. Give small
    // deviations a brief correction window, then let A* calculate a genuine
    // route from the player's real block if the corridor cannot be rejoined.
    if (outsideActiveCorridor && !waterDescentActive) {
        if (++ticksOutsidePath > 8) {
            clearInput(player);
            return ExecutionStatus::OffPath;
        }
    } else {
        ticksOutsidePath = 0;
    }

    // Airborne ticks are active progress for falls and jumps. Counting them as
    // stagnation causes false recovery on two-block drops near the jump apex.
    const bool preparingBridge = index < path.size() && path[index].movement == MovementType::Bridge;
    if (preparingBridge || waterDescentActive)
        ticksWithoutProgress = 0;
    if (!player->isOnGround()) {
        lastProgressPosition = feet;
        ticksWithoutProgress = 0;
    } else if (horizontalDistance(feet, lastProgressPosition) > 0.15f || std::abs(feet.y - lastProgressPosition.y) > 0.35f) {
        lastProgressPosition = feet;
        ticksWithoutProgress = 0;
    } else if (!preparingBridge && !waterDescentActive && ++ticksWithoutProgress > 80) {
        clearInput(player);
        return ExecutionStatus::Stuck;
    }

    const auto& node = path[index];
    const bool isBridge = node.movement == MovementType::Bridge;
    if (isBridge && activeBridgeIndex != index) {
        activeBridgeIndex = index;
        bridgeNextStep = 1;
        bridgePlacementWait = 0;
    }
    if (!isBridge)
        activeBridgeIndex = static_cast<std::size_t>(-1);
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
            if (along >= edgeThreshold && placeBridgeBlock(player, placementTarget, supportFace)) {
                bridgeNextStep = step + 1;
                bridgePlacementWait = 1;
            }
        }
    }
    const bool isParkour = node.movement == MovementType::Parkour;
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
        node.movement == MovementType::Diagonal;
    bool upcomingVerticalOrParkour = false;
    bool upcomingTurn = false;
    if (index > 0 && index + 1 < path.size()) {
        const auto& next = path[index + 1];
        const auto& source = path[index - 1].pos;
        const int currentX = std::clamp(node.pos.x - source.x, -1, 1);
        const int currentZ = std::clamp(node.pos.z - source.z, -1, 1);
        const int nextX = std::clamp(next.pos.x - node.pos.x, -1, 1);
        const int nextZ = std::clamp(next.pos.z - node.pos.z, -1, 1);
        upcomingTurn = currentX != nextX || currentZ != nextZ;
        upcomingVerticalOrParkour = next.movement == MovementType::Ascend ||
            next.movement == MovementType::Descend || next.movement == MovementType::Fall ||
            next.movement == MovementType::WaterDrop ||
            next.movement == MovementType::Parkour;
    }
    // On an exposed corner, Baritone-style safe-walk is preferable to trying
    // to compensate after momentum has already carried the player over air.
    float activeEdgeProgress = 0.f;
    if (index > 0)
        activeEdgeProgress = projectOntoSegment(feet, path[index - 1].pos, node.pos).progress;
    // Safe-walk only for the final portion of a genuinely one-wide corner.
    // The approach and all narrow straightaways remain uncrouched.
    const bool precisionSneak = narrowFooting && ordinaryMovement && upcomingTurn &&
        !upcomingVerticalOrParkour && activeEdgeProgress >= 0.58f &&
        player->isOnGround();
    if (isParkour) {
        if (activeParkourIndex != index) {
            resetParkourState();
            activeParkourIndex = index;
        }
        if (!player->isOnGround())
            parkourWasAirborne = true;
        // If the launch became airborne but this same edge is still active on
        // the next grounded tick, the destination was missed. Stop immediately
        // and let the controller replan instead of walking off another edge.
        if (parkourJumpIssued && parkourWasAirborne && player->isOnGround()) {
            clearInput(player);
            return ExecutionStatus::Stuck;
        }
        if (parkourJumpIssued && !parkourWasAirborne && ++parkourLaunchTicks > 10) {
            clearInput(player);
            return ExecutionStatus::Stuck;
        }
    } else {
        resetParkourState();
    }

    const glm::vec3 target{static_cast<float>(node.pos.x) + 0.5f, static_cast<float>(node.pos.y), static_cast<float>(node.pos.z) + 0.5f};
    glm::vec2 direction{target.x - feet.x, target.z - feet.z};
    glm::vec2 facingDirection = direction;
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
            if (parkourReady || parkourJumpIssued) {
                // Baritone keeps moving toward the destination throughout the
                // jump. Preserve the route tangent while correcting lateral
                // drift, rather than locking air input to a blind straight W.
                direction = jumpDirection - lateralOffset * (player->isOnGround() ? 0.65f : 1.10f);
                parkourLateralCorrection = lateral;
            } else {
                // PREPPING phase: return to the takeoff corridor before any
                // sprint or jump input is allowed.
                direction = sourceCenter - glm::vec2{feet.x, feet.z};
            }
        }
    }
    bool fallCoasting = false;
    bool fallBraking = false;
    if ((node.movement == MovementType::Descend || node.movement == MovementType::Fall ||
        node.movement == MovementType::WaterDrop) &&
        index > 0 && !player->isOnGround()) {
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
            const float currentAlongSpeed = std::max(alongSpeed, 0.f);
            const float predictedCoastProgress = projection.progress +
                bedrock_physics::projectedAirDisplacement(
                    currentAlongSpeed, 0.f, false, false, landingTicks) / fallLength;
            const float predictedBrakeProgress = projection.progress +
                bedrock_physics::projectedAirDisplacement(
                    currentAlongSpeed, -1.f, false, false, landingTicks) / fallLength;
            // A falling player has already accumulated most of the momentum
            // that will carry them forward. Forecast the actual no-input and
            // reverse-input landing points, then choose correction before the
            // body crosses the destination block instead of braking afterward.
            fallCoasting = projection.progress > 0.10f && predictedCoastProgress >= 0.62f;
            fallBraking = projection.progress > 0.18f && predictedCoastProgress >= 0.92f &&
                predictedBrakeProgress >= 0.92f;
        }
    }
    // Do not key this only off MovementType::Swim.  At the bottom of a water
    // column the planner can legitimately label the exit as Traverse/Descend
    // while the actor is still physically submerged.  The Bedrock liquid
    // volume is the authoritative signal for crouched downward movement.
    const bool approachingWaterColumn = waterDescentActive && !inWaterBlocks;
    const bool verticalSwimDown = waterDescentActive && inWaterBlocks;
    // Keep sneak held for the complete submerged stream segment.  The node
    // may briefly be level (or already at the exit Y), but releasing crouch
    // there makes Bedrock's current push the player out of the column.
    const bool waterStreamSegment = verticalSwimDown;
    const float distance = glm::length(direction);
    if (distance > 0.001f)
        direction /= distance;

    if (waterDescentActive) {
        // Water currents can push the player sideways out of a one-block
        // column. Keep the descent vertical while applying a strong, bounded
        // horizontal correction back toward the stream center.
        glm::vec2 centerDelta{
            static_cast<float>(waterColumnX) + 0.5f - feet.x,
            static_cast<float>(waterColumnZ) + 0.5f - feet.z};
        const float offset = glm::length(centerDelta);
        if (approachingWaterColumn && offset > 0.04f) {
            const glm::vec2 correction = centerDelta / std::max(offset, 0.001f);
            direction = correction * std::clamp(offset * 1.5f, 0.12f, 0.65f);
            facingDirection = direction;
        } else if (approachingWaterColumn) {
            direction = {};
        } else if (!approachingWaterColumn) {
            // Bedrock water preserves horizontal momentum after input is
            // released. Use a damped position controller: pull toward the
            // column centre and simultaneously oppose measured sideways
            // velocity. This actively arrests drift instead of waiting until
            // the player has already left the rendered line.
            const glm::vec2 horizontalVelocity{measuredMotion.x, measuredMotion.z};
            // Treat observed horizontal motion as the combined momentum and
            // stream push. A stronger derivative term counters the current
            // without calling an unstable native liquid helper or modifying
            // the player's velocity directly.
            glm::vec2 correction = centerDelta * 6.0f - horizontalVelocity * 14.f;
            const float correctionLength = glm::length(correction);
            const float horizontalSpeed = glm::length(horizontalVelocity);
            if (offset <= 0.018f && horizontalSpeed <= 0.005f) {
                direction = {};
            } else if (correctionLength > 0.001f) {
                // Flowing water can overpower a fractional analog correction.
                // Hold a full vanilla movement input toward the predicted
                // centre until both lateral error and drift have settled.
                direction = correction / correctionLength;
            }
        }
        // Centering can alternate by a few hundredths around the column
        // midpoint. Keep the view direction fixed during the descent so those
        // strafe corrections never become 180-degree head turns.
        if (!approachingWaterColumn) {
            const float fixedYaw = waterFacingYaw * std::numbers::pi_v<float> / 180.f;
            facingDirection = {-std::sin(fixedYaw), std::cos(fixedYaw)};
        }
    }

    // Use a short look-ahead tangent on ordinary ground segments. The search
    // remains block-based for collision correctness, while steering follows a
    // smooth arc through corners instead of making eight-direction snaps.
    if (!waterDescentActive && ordinaryMovement && index > 0 && index + 1 < path.size()) {
        const auto& source = path[index - 1].pos;
        const auto& next = path[index + 1];
        if ((next.movement == MovementType::Traverse || next.movement == MovementType::Diagonal) &&
            next.pos.y == node.pos.y) {
            glm::vec2 currentTangent{static_cast<float>(node.pos.x - source.x),
                static_cast<float>(node.pos.z - source.z)};
            glm::vec2 nextTangent{static_cast<float>(next.pos.x - node.pos.x),
                static_cast<float>(next.pos.z - node.pos.z)};
            if (glm::length(currentTangent) > 0.001f && glm::length(nextTangent) > 0.001f) {
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
        ? std::clamp(distance / 1.15f, 0.82f, 1.f)
        : 1.f;
    const float waterApproachScale = approachingWaterColumn
        ? std::clamp(glm::length(glm::vec2{
              static_cast<float>(waterColumnX) + 0.5f - feet.x,
              static_cast<float>(waterColumnZ) + 0.5f - feet.z}) * 1.15f, 0.f, 0.75f)
        : 1.f;
    const float facingDistance = glm::length(facingDirection);
    if (facingDistance > 0.001f)
        facingDirection /= facingDistance;

    // Keep the actor facing the rendered path for the entire route. Movement is
    // resolved against this same yaw below, so W is genuinely forward and the
    // native sprint state no longer drops when the user's camera pointed away.
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
        rotation.y = desiredYaw;
        player->setRotation(rotation);
    }

    const float yaw = player->getRotation().y * std::numbers::pi_v<float> / 180.f;
    const glm::vec2 forward{-std::sin(yaw), std::cos(yaw)};
    const glm::vec2 right{-forward.y, forward.x};
    const float parkourLength = static_cast<float>(std::max(parkourDistance, 1));
    const float requestedForward = std::clamp(glm::dot(forward, direction), -1.f, 1.f);
    const bool parkourNeedsSprint = isParkour && (parkourDistance >= 4 || parkourAscend);

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
    const float minimumLaunchSpeed = parkourDistance >= 4 ? 0.20f : (parkourAscend ? 0.17f : 0.f);
    const bool sprintReady = !parkourNeedsSprint || parkourSprintPrimed ||
        measuredApproachSpeed >= minimumLaunchSpeed || nextApproachSpeed >= minimumLaunchSpeed;
    const bool requestParkourJump = isParkour && parkourReady && !parkourJumpIssued && sprintReady &&
        nextParkourAlong >= takeoffDistance && requestedForward > 0.92f;
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

    float cautiousScale = 1.f;
    if (!isParkour) {
        if (precisionSneak)
            cautiousScale = 0.82f;
    }
    const float pathMovementScale = waterDescentActive
        ? 1.f
        : cautiousScale * humanApproachScale;
    const float forwardAmount = parkourAirBrake ? -0.24f :
        (parkourAirRelease ? 0.f : (parkourApproachBrake ? -0.16f :
        (parkourApproachRelease ? 0.f :
        (fallBraking ? -0.55f : (fallCoasting ? 0.f :
            requestedForward * pathMovementScale * waterApproachScale)))));
    // Keep lateral correction active while braking so an imperfect launch is
    // pulled back over the landing block instead of drifting beside it.
    const float leftAmount = std::clamp(-glm::dot(right, direction), -1.f, 1.f) *
        ((parkourAirRelease || parkourAirBrake) ?
            std::clamp(parkourLateralCorrection * 2.5f, 0.25f, 1.f) : 1.f);
    const glm::vec2 localMovement{leftAmount, forwardAmount};

    if (const auto input = player->tryGet<MoveInputComponent>()) {
        if (!movementModeCaptured) {
            previousCameraRelativeMovement = input->isCameraRelativeMovementEnabled;
            previousRotationControlledByMovement = input->isRotControlledByMoveDirection;
            movementModeCaptured = true;
        }
        // Use the actor/path yaw for all movement. Leaving this camera-relative
        // is what made the bot strafe or walk backward and lose sprint.
        input->isCameraRelativeMovementEnabled = false;
        input->isRotControlledByMoveDirection = true;

        input->move = localMovement;
        input->inputState.analogMoveVector = localMovement;
        input->rawInputState.analogMoveVector = localMovement;

        constexpr float digitalThreshold = 0.35f;
        input->inputState.up = forwardAmount > digitalThreshold;
        input->inputState.down = forwardAmount < -digitalThreshold;
        input->inputState.left = leftAmount > digitalThreshold;
        input->inputState.right = leftAmount < -digitalThreshold;
        input->rawInputState.up = input->inputState.up;
        input->rawInputState.down = input->inputState.down;
        input->rawInputState.left = input->inputState.left;
        input->rawInputState.right = input->inputState.right;

        // Java Baritone only forces sprint for the maximum-distance or ascending
        // parkour variants. Sprinting on short gaps causes Bedrock to overshoot.
        // Bedrock may internally limit the final speed while crouched, but keep
        // sprint requested during safe-walk as configured instead of forcibly
        // clearing it. Wide ordinary paths run at full vanilla speed.
        const bool sprintSafe = isParkour ||
            (ordinaryMovement && !upcomingVerticalOrParkour && (!narrowFooting || precisionSneak));
        const float sprintThreshold = precisionSneak ? 0.45f : (isParkour ? 0.55f : 0.8f);
        const bool shouldSprint = options.sprint && forwardAmount > sprintThreshold &&
            (!isParkour || parkourNeedsSprint) && !parkourAirRelease &&
            sprintSafe && !player->isInWater() && !waterDescentActive;
        input->inputState.sprintDown = shouldSprint;
        input->rawInputState.sprintDown = shouldSprint;
        input->sprinting = shouldSprint;
        if (isBridge) {
            input->sneaking = true;
            input->persistSneak = true;
            input->inputState.sneakDown = true;
            input->rawInputState.sneakDown = true;
        }
        if (waterStreamSegment) {
            input->inputState.sneakInputCurrentlyDown = true;
            input->rawInputState.sneakInputCurrentlyDown = true;
        }
        // Let Bedrock's normal movement systems consume the inputs. Locking
        // this component and writing velocity directly causes server lagbacks.
        input->moveInputStateLocked = false;

        if (isParkour && parkourReady && parkourNeedsSprint && shouldSprint) {
            ++parkourSprintTicks;
            // Require a short native sprint runway, but accept measured speed
            // immediately when the player entered this edge with momentum.
            parkourSprintPrimed = parkourSprintTicks >= 2 ||
                measuredApproachSpeed >= minimumLaunchSpeed;
        }
    }

    const bool shouldJump = (node.movement == MovementType::Ascend && target.y > feet.y + 0.2f) ||
        (node.movement == MovementType::WaterDrop && player->isOnGround()) || requestParkourJump;
    const bool shouldSwimUp = node.movement == MovementType::Swim && target.y > feet.y + 0.15f;
    const bool shouldSwimDown = verticalSwimDown ||
        (node.movement == MovementType::Swim && target.y < feet.y - 0.05f);
    // Pulse jump while grounded instead of holding it throughout the flight.
    // This also prevents an immediate second jump on the landing tick.
    const bool holdJump = (shouldJump && player->isOnGround()) || shouldSwimUp;
    if (holdJump)
        player->jumpFromGround();
    if (requestParkourJump) {
        parkourJumpIssued = true;
        parkourLaunchTicks = 0;
    }
    if (const auto input = player->tryGet<MoveInputComponent>()) {
        input->jumping = holdJump;
        input->inputState.jumpDown = holdJump;
        input->inputState.jumpInputCurrentlyDown = holdJump;
        input->rawInputState.jumpDown = holdJump;
        input->rawInputState.jumpInputCurrentlyDown = holdJump;
    }

    if (const auto input = player->tryGet<MoveInputComponent>()) {
        // A vertical downward water-stream node intentionally crouches: this
        // is the vanilla fast-descent behavior the player uses in bubbleless
        // water columns. Ordinary submerged travel remains uncrouched.
        const bool shouldSneak = isBridge || waterStreamSegment || precisionSneak;
        input->sneaking = shouldSneak;
        input->wantDown = shouldSwimDown;
        input->inputState.sneakDown = shouldSneak;
        input->rawInputState.sneakDown = shouldSneak;
    }

    controlledMovement = true;
    return ExecutionStatus::Running;
}

void PathExecutor::stop(LocalPlayer* player) {
    clearInput(player);
    path.clear();
    index = 0;
    ticksWithoutProgress = 0;
    ticksOutsidePath = 0;
    progressInitialized = false;
    pathRotationActive = false;
    visualYawInitialized = false;
    cameraYawCaptured = false;
    renderRotationOverrideActive = false;
    lastRotationRenderMillis = 0;
    resetParkourState();
    bridgeNextStep = 1;
    bridgePlacementWait = 0;
    activeBridgeIndex = static_cast<std::size_t>(-1);
    waterDescentActive = false;
    waterBottomKnown = false;
    waterExitIndex = static_cast<std::size_t>(-1);
    waterDescentTicks = 0;
    waterVerticalSettledTicks = 0;
    waterHasDescended = false;
    bridgePitchActive = false;
    bridgePitch = 0.f;
}

bool PathExecutor::placeBridgeBlock(LocalPlayer* player, const BlockPos& target, const FacingID preferredFace) {
    auto* source = MC::getRegion();
    if (player == nullptr || source == nullptr) return false;
    const glm::ivec3 pos{target.x, target.y, target.z};
    auto* existing = source->getBlock(pos);
    if (existing != nullptr && existing->getBlockLegacy() != nullptr && existing->getBlockLegacy()->isSolid()) return true;
    auto* supplies = player->getSupplies();
    if (supplies == nullptr || supplies->getInventory() == nullptr) return false;
    bool hasBlock = false;
    for (int slot = 0; slot < 9; ++slot) {
        auto* stack = supplies->getInventory()->getItem(slot);
        if (stack != nullptr && stack->isValid() && stack->getItem() != nullptr && stack->getItem()->isBlock()) {
            supplies->setSelectedHotbarSlot(slot);
            hasBlock = true;
            break;
        }
    }
    if (!hasBlock || player->getGameMode() == nullptr) return false;
    static constexpr glm::ivec3 supports[] = {{0,-1,0},{0,1,0},{0,0,-1},{0,0,1},{-1,0,0},{1,0,0}};
    for (int face = 0; face < 6; ++face) {
        const auto support = pos + supports[face];
        auto* block = source->getBlock(support);
        if (block == nullptr || block->getBlockLegacy() == nullptr || !block->getBlockLegacy()->isSolid()) continue;
        auto place = pos;
        if (player->getGameMode()->buildBlock(&place, static_cast<FacingID>(face), false)) return true;
    }
    if (preferredFace != FacingID::Unknown) {
        auto place = pos;
        if (player->getGameMode()->buildBlock(&place, preferredFace, false)) return true;
    }
    return false;
}

void PathExecutor::suspend(LocalPlayer* player) {
    clearInput(player);
    pathRotationActive = false;
    visualYawInitialized = false;
}

void PathExecutor::applyVisualRotation(LocalPlayer* player) {
    // Physics/networking consumed the exact movement yaw. Restore the native
    // camera yaw immediately afterward; Limiter server rotation never forces the
    // user's camera to follow its server-facing rotation.
    if (player == nullptr || !pathRotationActive || !cameraYawCaptured)
        return;

    auto rotation = player->getRotation();
    rotation.y = cameraYaw;
    player->setRotation(rotation);
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
    if (bridgePitchActive) {
        visualPitch += (bridgePitchTarget - visualPitch) *
            std::clamp(delta * 7.0f, 0.f, 1.f);
        auto renderRotation = savedActorRotation;
        renderRotation.x = visualPitch;
        player->setRotation(renderRotation);
    } else {
        visualPitch = savedActorRotation.x;
    }
    if (auto* body = player->tryGet<MobBodyRotationComponent>()) {
        savedBodyRotation = body->bodyRotation;
        savedPreviousBodyRotation = body->previousBodyRotation;
        body->bodyRotation = visualBodyYaw;
        body->previousBodyRotation = visualBodyYaw;
    }
    renderRotationOverrideActive = true;
}

void PathExecutor::endVisualRotationRender(LocalPlayer* player) {
    if (player == nullptr || !renderRotationOverrideActive)
        return;
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
    if (player == nullptr || !controlledMovement)
        return;

    if (bridgePitchActive) {
        auto rotation = player->getRotation();
        rotation.x = savedBridgePitch;
        player->setRotation(rotation);
    bridgePitchActive = false;
    bridgePitchTarget = 62.f;
}

    if (const auto input = player->tryGet<MoveInputComponent>()) {
        if (movementModeCaptured) {
            input->isCameraRelativeMovementEnabled = previousCameraRelativeMovement;
            input->isRotControlledByMoveDirection = previousRotationControlledByMovement;
            movementModeCaptured = false;
        }
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
        input->rawInputState.up = false;
        input->rawInputState.down = false;
        input->rawInputState.left = false;
        input->rawInputState.right = false;
        input->rawInputState.sprintDown = false;
        input->rawInputState.jumpDown = false;
        input->rawInputState.jumpInputCurrentlyDown = false;
        input->inputState.sneakDown = false;
        input->rawInputState.sneakDown = false;
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
    parkourSprintPrimed = false;
    parkourSprintTicks = 0;
    parkourLaunchTicks = 0;
}

std::size_t PathExecutor::getCurrentIndex() const { return index; }

const std::vector<PathNode>& PathExecutor::getPath() const { return path; }

} // namespace baritone
