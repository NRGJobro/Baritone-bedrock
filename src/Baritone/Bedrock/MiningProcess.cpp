#include "MiningProcess.h"

#include "../BaritoneController.h"
#include "../Core/AdvancedGoals.h"
#include "../Core/Movement.h"
#include "BedrockWorld.h"
#include "../../SDK/MC.h"
#include "../../SDK/Client/ClientInstance.h"
#include "../../SDK/Core/Minecraft.h"
#include "../../SDK/World/Actor/GameMode.h"
#include "../../SDK/World/Actor/LocalPlayer.h"
#include "../../SDK/World/Actor/Components/AABBShapeComponent.h"
#include "../../SDK/World/Actor/Components/ActorOwnerComponent.h"
#include "../../SDK/World/Actor/Components/ActorTypeComponent.h"
#include "../../SDK/World/Block/Block.h"
#include "../../SDK/World/Block/BlockLegacy.h"
#include "../../SDK/World/BlockSource.h"
#include "../../SDK/World/Inventory/Inventory.h"
#include "../../SDK/World/Inventory/PlayerInventory.h"
#include "../../SDK/World/Item/ItemStack.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <string_view>
#include <utility>

namespace baritone {
namespace {

struct ActorSnapshot {
    std::uint32_t entityId = 0;
    glm::vec3 position{};
    bool pickupSized = false;
};

std::vector<ActorSnapshot> snapshotActors(bool* registryAvailable = nullptr) {
    std::vector<ActorSnapshot> result;
    auto* client = MC::getClientInstance();
    auto* minecraft = client == nullptr ? nullptr : client->getMinecraft();
    auto registry = minecraft == nullptr ? nullptr : minecraft->getEntityRegistry();
    if (registryAvailable != nullptr)
        *registryAvailable = registry != nullptr;
    if (registry == nullptr)
        return result;

    auto view = registry->ownedRegistry.view<ActorOwnerComponent>();
    result.reserve(view.size_hint());
    for (const auto entityId : view) {
        auto& owner = view.get<ActorOwnerComponent>(entityId);
        auto* actor = owner.entity.get();
        auto* state = actor == nullptr ? nullptr : actor->tryGet<StateVectorComponent>();
        if (state == nullptr)
            continue;

        const auto* shape = actor->tryGet<AABBShapeComponent>();
        const auto* actorType = actor->tryGet<ActorTypeComponent>();
        // Track actual dropped-item actors only. Experience orbs are also
        // compact and spawn beside mined ore, but following one can make the
        // miner believe the item was collected while the diamond remains.
        constexpr int itemActorType = 64;
        const bool pickupSized = actorType != nullptr && actorType->type == itemActorType &&
            shape != nullptr && shape->size.x <= 0.90f && shape->size.y <= 0.90f;
        result.push_back({entityId.rawId, state->pos, pickupSized});
    }
    return result;
}

BlockPos actorBlock(const glm::vec3& position) {
    return {static_cast<int>(std::floor(position.x)),
        static_cast<int>(std::floor(position.y)),
        static_cast<int>(std::floor(position.z))};
}

double distanceSquared(const BlockPos& left, const BlockPos& right) {
    const double dx = static_cast<double>(left.x - right.x);
    const double dy = static_cast<double>(left.y - right.y);
    const double dz = static_cast<double>(left.z - right.z);
    return dx * dx + dy * dy + dz * dz;
}

BlockPos playerFeetBlock(LocalPlayer* player) {
    if (player == nullptr)
        return {};
    const auto feet = player->getFeetPosition();
    return {static_cast<int>(std::floor(feet.x)), static_cast<int>(std::floor(feet.y + 0.1251f)),
        static_cast<int>(std::floor(feet.z))};
}

BlockLegacy* blockLegacyAt(BlockSource* region, const BlockPos& pos) {
    if (region == nullptr)
        return nullptr;
    auto* block = region->getBlock(pos.x, pos.y, pos.z);
    return block == nullptr ? nullptr : block->getBlockLegacy();
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

float distanceToBlock(const glm::vec3& point, const BlockPos& block) {
    const float nearestX = std::clamp(point.x, static_cast<float>(block.x), static_cast<float>(block.x + 1));
    const float nearestY = std::clamp(point.y, static_cast<float>(block.y), static_cast<float>(block.y + 1));
    const float nearestZ = std::clamp(point.z, static_cast<float>(block.z), static_cast<float>(block.z + 1));
    return glm::distance(point, glm::vec3{nearestX, nearestY, nearestZ});
}

bool isVisibleFromPlayer(LocalPlayer* player, BlockSource* region, const BlockPos& target) {
    if (player == nullptr || region == nullptr)
        return false;
    const glm::vec3 eye = player->getPosition();
    const glm::vec3 targetCenter{target.x + 0.5f, target.y + 0.5f, target.z + 0.5f};
    const std::array<glm::vec3, 7> samplePoints{{
        targetCenter,
        {target.x + 0.08f, target.y + 0.5f, target.z + 0.5f},
        {target.x + 0.92f, target.y + 0.5f, target.z + 0.5f},
        {target.x + 0.5f, target.y + 0.08f, target.z + 0.5f},
        {target.x + 0.5f, target.y + 0.92f, target.z + 0.5f},
        {target.x + 0.5f, target.y + 0.5f, target.z + 0.08f},
        {target.x + 0.5f, target.y + 0.5f, target.z + 0.92f}}};
    for (const auto& sample : samplePoints) {
        if (distanceToBlock(eye, target) > 6.10f)
            continue;
        const glm::vec3 ray = sample - eye;
        const float length = glm::length(ray);
        const int steps = std::max(1, static_cast<int>(std::ceil(length / 0.05f)));
        bool blocked = false;
        for (int step = 1; step < steps; ++step) {
            const float fraction = static_cast<float>(step) / static_cast<float>(steps);
            const glm::vec3 point = eye + ray * fraction;
            const BlockPos cell{static_cast<int>(std::floor(point.x)),
                static_cast<int>(std::floor(point.y)), static_cast<int>(std::floor(point.z))};
            if (cell == target)
                break;
            const auto block = BedrockWorld(region).getBlock(cell);
            if (!block.loaded || block.solid || block.liquid || block.hazard) {
                blocked = true;
                break;
            }
        }
        if (!blocked)
            return true;
    }
    return false;
}

BlockPos estimateDropLanding(const IWorld& world, const BlockPos& minedBlock) {
    // Block drops spawn around the removed block and then obey gravity. Find
    // the first loaded solid floor below so pickup pathing follows the item to
    // the floor instead of walking to a now-empty ceiling ore cell.
    constexpr int maximumDropSearch = 48;
    for (int drop = 0; drop <= maximumDropSearch; ++drop) {
        const auto cell = minedBlock.offset(0, -drop, 0);
        const auto state = world.getBlock(cell);
        if (!state.loaded)
            break;
        // Items float to the top of water. Target the air cell over the first
        // liquid surface encountered so the normal water-only bridge movement
        // can collect the drop without asking the player to enter the liquid.
        // Lava remains unreachable because hazard-aware pathing will reject
        // that collection route and stop instead of sacrificing the player.
        if (state.liquid)
            return cell.offset(0, 1, 0);
        if (state.solid)
            return cell.offset(0, 1, 0);
    }
    return minedBlock;
}

} // namespace

void MiningProcess::start(std::vector<int> ids, std::vector<std::string> names, const int quantity,
    const int radius, const bool continueMode) {
    std::erase_if(ids, [](const int id) { return id <= 0; });
    std::ranges::sort(ids);
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    for (auto& name : names) {
        std::ranges::transform(name, name.begin(), [](const unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (name.starts_with("minecraft:"))
            name.erase(0, 10);
        // Be forgiving with the shorthand commonly used in chat. The Java
        // client accepts block families, so `.mine diamond` should behave the
        // same as `.mine diamond_ore`.
        if (name == "diamond" || name == "diamondore")
            name = "diamond_ore";
    }
    std::erase_if(names, [](const auto& name) { return name.empty(); });
    // Match Java Baritone's block-family behavior for the common command: a
    // request for diamond_ore means both stone and deepslate variants.
    if (std::ranges::find(names, "diamond_ore") != names.end() &&
        std::ranges::find(names, "deepslate_diamond_ore") == names.end())
        names.emplace_back("deepslate_diamond_ore");
    std::ranges::sort(names);
    names.erase(std::unique(names.begin(), names.end()), names.end());
    blockIds = std::move(ids);
    blockNames = std::move(names);
    fixedTargets.clear();
    fixedTargetMode = false;
    activePatch.clear();
    // `continue` intentionally overrides a finite count and keeps scanning
    // for newly loaded matching blocks after each collected drop.
    continueMining = continueMode;
    desiredQuantity = continueMode ? 0 : std::max(0, quantity);
    minedQuantity = 0;
    scanRadius = std::clamp(radius, 4, 64);
    candidates.clear();
    blacklist.clear();
    noVisibleTargetPosition.reset();
    breakingTarget.reset();
    pendingPickupTargets.clear();
    pendingPickupActors.clear();
    knownActorIds.clear();
    pickupActor.reset();
    pickupTarget.reset();
    pickupGoal.reset();
    breakingTargetCountsGoal = true;
    pendingCompletion = false;
    collectingDrops = false;
    actorSnapshotInitialized = false;
    bool registryAvailable = false;
    for (const auto& actor : snapshotActors(&registryAvailable))
        knownActorIds.insert(actor.entityId);
    actorSnapshotInitialized = registryAvailable;
    pickupTicks = 0;
    pickupPathAttempts = 0;
    breakTicks = 0;
    previousHotbarSlot = -1;
    active = !blockIds.empty() || !blockNames.empty();
    pendingMessage = active ? std::optional<std::string>{"Mining process started."} :
        std::optional<std::string>{"Mining requires at least one positive block ID."};
}

void MiningProcess::startTargets(std::vector<BlockPos> targets) {
    std::ranges::sort(targets, [](const auto& left, const auto& right) {
        if (left.y != right.y) return left.y < right.y;
        if (left.x != right.x) return left.x < right.x;
        return left.z < right.z;
    });
    targets.erase(std::unique(targets.begin(), targets.end()), targets.end());
    blockIds.clear();
    blockNames.clear();
    fixedTargets = std::move(targets);
    fixedTargetMode = true;
    continueMining = false;
    activePatch.clear();
    desiredQuantity = static_cast<int>(fixedTargets.size());
    minedQuantity = 0;
    candidates.clear();
    blacklist.clear();
    noVisibleTargetPosition.reset();
    breakingTarget.reset();
    pendingPickupTargets.clear();
    pendingPickupActors.clear();
    knownActorIds.clear();
    pickupActor.reset();
    pickupTarget.reset();
    pickupGoal.reset();
    pendingCompletion = false;
    collectingDrops = false;
    actorSnapshotInitialized = false;
    bool registryAvailable = false;
    for (const auto& actor : snapshotActors(&registryAvailable))
        knownActorIds.insert(actor.entityId);
    actorSnapshotInitialized = registryAvailable;
    pickupTicks = 0;
    pickupPathAttempts = 0;
    breakingTargetCountsGoal = true;
    breakTicks = 0;
    previousHotbarSlot = -1;
    active = !fixedTargets.empty();
    pendingMessage = active ? std::optional<std::string>{"Tunnel clearing process started."} :
        std::optional<std::string>{"Tunnel contains no blocks to clear."};
}

void MiningProcess::cancel(BaritoneController& controller) {
    if (breakingTarget) {
        if (const auto player = MC::getLocalPlayer(); player != nullptr && player->getGameMode() != nullptr) {
            const auto& target = *breakingTarget;
            player->getGameMode()->stopDestroyBlock({target.x, target.y, target.z});
        }
    }
    active = false;
    candidates.clear();
    activePatch.clear();
    noVisibleTargetPosition.reset();
    breakingTarget.reset();
    pendingPickupTargets.clear();
    pendingPickupActors.clear();
    knownActorIds.clear();
    pickupActor.reset();
    pickupTarget.reset();
    pickupGoal.reset();
    collectingDrops = false;
    actorSnapshotInitialized = false;
    pickupTicks = 0;
    pickupPathAttempts = 0;
    restoreHotbar();
    controller.stop();
    restoreBreakPolicy(controller);
}

void MiningProcess::tick(BaritoneController& controller) {
    if (!active)
        return;

    // Unlike ordinary .goto navigation, mining is allowed to include safe,
    // breakable feet/head blocks in its route. Restore the user's navigation
    // policy as soon as this process ends.
    enableBreakPolicy(controller);

    auto* player = MC::getLocalPlayer();
    auto* region = MC::getRegion();
    if (player == nullptr || region == nullptr || player->getGameMode() == nullptr)
        return;

    const BedrockWorld world(region);

    // Associate newly spawned, item-sized actors with the closest recently
    // mined block. Their live positions are much more reliable than assuming
    // every drop falls straight down: drops can bounce onto a ledge, float in
    // moving water, or merge after the block disappears.
    bool registryAvailable = false;
    const auto actors = snapshotActors(&registryAvailable);
    if (!actorSnapshotInitialized) {
        for (const auto& actor : actors)
            knownActorIds.insert(actor.entityId);
        actorSnapshotInitialized = registryAvailable;
    } else {
        for (const auto& actor : actors) {
            if (knownActorIds.contains(actor.entityId))
                continue;
            if (!actor.pickupSized) {
                knownActorIds.insert(actor.entityId);
                continue;
            }

            std::optional<BlockPos> nearestSource;
            float nearestDistance = 3.25f;
            for (const auto& source : pendingPickupTargets) {
                const float distance = distanceToBlock(actor.position, source);
                if (distance <= nearestDistance) {
                    nearestDistance = distance;
                    nearestSource = source;
                }
            }
            if (pickupTarget) {
                const float distance = distanceToBlock(actor.position, *pickupTarget);
                if (distance <= nearestDistance) {
                    nearestDistance = distance;
                    nearestSource = *pickupTarget;
                }
            }
            if (nearestSource) {
                knownActorIds.insert(actor.entityId);
                pendingPickupActors.push_back({actor.entityId, *nearestSource});
            }
            // Do not mark an unmatched small actor as known yet. The item can
            // spawn during the same game tick in which the source block is
            // first observed as air; the source is queued later in this tick
            // and will be available for association on the next one.
        }
    }

    const auto findActor = [&](const std::uint32_t entityId) {
        return std::ranges::find_if(actors, [&](const auto& actor) {
            return actor.entityId == entityId;
        });
    };
    // If a queued actor has already vanished, Bedrock has either pulled it
    // into the inventory/XP bar or removed it while we were still beside the
    // patch. It no longer needs a collection route.
    std::erase_if(pendingPickupActors, [&](const auto& actor) {
        return findActor(actor.entityId) == actors.end();
    });

    // Mine every block that is already reachable from this position before
    // entering collection mode. As soon as continuing would require another
    // path (or the requested quantity is complete), sweep the accumulated
    // drop positions first. This preserves patch batching without walking
    // away from items that are still on the floor.
    if (!pickupTarget && (!pendingPickupTargets.empty() || !pendingPickupActors.empty())) {
        bool canKeepMiningHere = false;
        if (!pendingCompletion && !collectingDrops) {
            const auto playerPosition = player->getPosition();
            canKeepMiningHere = std::ranges::any_of(activePatch, [&](const auto& target) {
                return matches(blockLegacyAt(region, target)) &&
                    distanceToBlock(playerPosition, target) <= 6.10f &&
                    isVisibleFromPlayer(player, region, target);
            });
        }

        if (pendingCompletion || collectingDrops || activePatch.empty() || !canKeepMiningHere) {
            collectingDrops = true;
            const auto playerBlock = playerFeetBlock(player);
            if (!pendingPickupActors.empty()) {
                auto closest = pendingPickupActors.begin();
                double closestDistance = std::numeric_limits<double>::infinity();
                for (auto iterator = pendingPickupActors.begin(); iterator != pendingPickupActors.end(); ++iterator) {
                    const auto actor = findActor(iterator->entityId);
                    if (actor == actors.end())
                        continue;
                    const auto landing = estimateDropLanding(world, actorBlock(actor->position));
                    const auto distance = distanceSquared(playerBlock, landing);
                    if (distance < closestDistance) {
                        closestDistance = distance;
                        closest = iterator;
                    }
                }
                pickupActor = *closest;
                pickupTarget = closest->source;
                const auto actor = findActor(closest->entityId);
                pickupGoal = actor == actors.end() ? estimateDropLanding(world, *pickupTarget) :
                    estimateDropLanding(world, actorBlock(actor->position));
                pendingPickupActors.erase(closest);
                if (const auto source = std::ranges::find(pendingPickupTargets, *pickupTarget);
                    source != pendingPickupTargets.end())
                    pendingPickupTargets.erase(source);
            } else {
                auto closest = pendingPickupTargets.begin();
                double closestDistance = std::numeric_limits<double>::infinity();
                for (auto iterator = pendingPickupTargets.begin(); iterator != pendingPickupTargets.end(); ++iterator) {
                    const auto landing = estimateDropLanding(world, *iterator);
                    const auto distance = distanceSquared(playerBlock, landing);
                    if (distance < closestDistance) {
                        closestDistance = distance;
                        closest = iterator;
                    }
                }
                pickupActor.reset();
                pickupTarget = *closest;
                pendingPickupTargets.erase(closest);
                pickupGoal = estimateDropLanding(world, *pickupTarget);
            }
            pickupTicks = 0;
            pickupPathAttempts = 0;
            controller.stop();
        }
    }

    // Follow an observed drop to its real landing cell. If this Bedrock build
    // does not expose a matching item actor, use the conservative source/fall
    // estimate instead. Both routes retain mining mode, so safe solid feet and
    // head blocks are cleared on the way to the pickup.
    if (pickupTarget) {
        auto activeActor = actors.end();
        if (pickupActor)
            activeActor = findActor(pickupActor->entityId);

        // A tracked actor disappearing is the positive pickup signal. Do not
        // wait out a guessed timer or path elsewhere after this point.
        if (pickupActor && activeActor == actors.end()) {
            pickupActor.reset();
            pickupTarget.reset();
            pickupGoal.reset();
            pickupTicks = 0;
            pickupPathAttempts = 0;
            controller.stop();
            if (pendingPickupTargets.empty() && pendingPickupActors.empty())
                collectingDrops = false;
            if (pendingCompletion && pendingPickupTargets.empty() && pendingPickupActors.empty()) {
                pendingCompletion = false;
                active = false;
                restoreHotbar();
                restoreBreakPolicy(controller);
                pendingMessage = "Mining complete: " + std::to_string(minedQuantity) +
                    " block(s), with drops collected.";
            }
            return;
        }

        if (activeActor != actors.end()) {
            const auto liveGoal = estimateDropLanding(world, actorBlock(activeActor->position));
            if (!pickupGoal || *pickupGoal != liveGoal) {
                pickupGoal = liveGoal;
                pickupTicks = 0;
                pickupPathAttempts = 0;
                controller.stop();
            }
        } else if (!pickupGoal) {
            pickupGoal = estimateDropLanding(world, *pickupTarget);
        }

        // Distance alone is not enough here: a drop in a diagonally adjacent
        // cell can be less than 1.2 blocks away while a corner wall completely
        // prevents collection. Require the player to enter the drop's cell so
        // the controller must walk or mine all the way to it.
        const auto playerBlock = playerFeetBlock(player);
        const bool atPickupCell = playerBlock == *pickupGoal;
        if (atPickupCell) {
            if (controller.getState() != ControllerState::Idle)
                controller.stop();
            ++pickupTicks;
            if (pickupActor) {
                // The live item actor disappearing is the pickup signal. Stay
                // in its exact cell until that happens; never infer a full
                // inventory merely because the client retained it for a few
                // seconds.
                return;
            }

            const int fallDistance = std::max(0, pickupTarget->y - pickupGoal->y);
            const int requiredPickupTicks = std::clamp(30 + fallDistance * 2, 30, 120);
            if (pickupTicks < requiredPickupTicks)
                return;

            // Standing here also collects drops from adjacent vein blocks.
            // Remove those nearby waypoints so a four-block patch usually
            // needs one collection stop rather than four path calculations.
            std::erase_if(pendingPickupTargets, [&](const auto& target) {
                const auto landing = estimateDropLanding(world, target);
                return landing == playerBlock;
            });
            pickupActor.reset();
            pickupTarget.reset();
            pickupGoal.reset();
            pickupTicks = 0;
            pickupPathAttempts = 0;
            controller.stop();
            if (pendingPickupTargets.empty() && pendingPickupActors.empty()) {
                collectingDrops = false;
            }
            if (pendingCompletion && pendingPickupTargets.empty() && pendingPickupActors.empty()) {
                pendingCompletion = false;
                active = false;
                restoreHotbar();
                restoreBreakPolicy(controller);
                pendingMessage = "Mining complete: " + std::to_string(minedQuantity) +
                    " block(s), with drops collected.";
            }
            return;
        }

        pickupTicks = 0;
        if (controller.getState() == ControllerState::Calculating ||
            controller.getState() == ControllerState::Executing ||
            controller.getState() == ControllerState::Paused)
            return;

        // Try the live landing cell first, then its immediate pickup radius,
        // and finally the original mined cell. Every attempt is a mining-mode
        // route, so an enclosed drop gets a tunnel instead of a walk-only goal.
        if (controller.getState() == ControllerState::Failed ||
            controller.getState() == ControllerState::Arrived)
            controller.stop();
        if (pickupPathAttempts >= 6) {
            controller.stop();
            active = false;
            restoreHotbar();
            restoreBreakPolicy(controller);
            pendingMessage = "Mining stopped because the drop is sealed behind unsafe or unbreakable terrain.";
            return;
        }

        ++pickupPathAttempts;
        const auto liveCell = activeActor == actors.end() ? *pickupTarget : actorBlock(activeActor->position);
        const auto landing = activeActor == actors.end() ? estimateDropLanding(world, *pickupTarget) :
            estimateDropLanding(world, liveCell);
        std::shared_ptr<Goal> collectionGoal;
        if (pickupPathAttempts == 1) {
            pickupGoal = landing;
            collectionGoal = std::make_shared<GoalBlock>(landing);
        } else if (pickupPathAttempts == 2) {
            pickupGoal = landing;
            collectionGoal = std::make_shared<GoalNear>(landing, 1);
        } else if (pickupPathAttempts == 3 && activeActor != actors.end()) {
            pickupGoal = landing;
            collectionGoal = std::make_shared<GoalBlock>(liveCell);
        } else if (pickupPathAttempts == 4 && activeActor != actors.end()) {
            pickupGoal = landing;
            collectionGoal = std::make_shared<GoalNear>(liveCell, 1);
        } else if (pickupPathAttempts == 5) {
            collectionGoal = std::make_shared<GoalBlock>(*pickupTarget);
        } else {
            collectionGoal = std::make_shared<GoalNear>(*pickupTarget, 1);
        }
        if (!controller.goTo(std::move(collectionGoal))) {
            controller.stop();
            active = false;
            restoreHotbar();
            restoreBreakPolicy(controller);
            pendingMessage = "Mining stopped because the drop pickup route could not start.";
        }
        return;
    }

    if (breakingTarget) {
        const auto target = *breakingTarget;
        const auto currentTargetState = world.getBlock(target);
        if (!currentTargetState.loaded) {
            if (breakTicks > 0)
                player->getGameMode()->stopDestroyBlock({target.x, target.y, target.z});
            breakingTarget.reset();
            breakTicks = 0;
            controller.stop();
            return;
        }
        if (currentTargetState.solid && breakingTargetCountsGoal &&
            !matches(blockLegacyAt(region, target))) {
            if (breakTicks > 0)
                player->getGameMode()->stopDestroyBlock({target.x, target.y, target.z});
            breakingTarget.reset();
            breakTicks = 0;
            controller.stop();
            return;
        }
        if (!currentTargetState.solid) {
            const bool wasGoal = breakingTargetCountsGoal;
            breakingTargetCountsGoal = true;
            if (!wasGoal) {
                // A route-clearance block is intentionally not part of the
                // requested quantity. Re-evaluate the same ore patch now that
                // its line of sight has been opened.
                breakingTarget.reset();
                breakTicks = 0;
                noVisibleTargetPosition.reset();
                pendingPickupTargets.push_back(target);
                controller.stop();
                return;
            }
            ++minedQuantity;
            std::erase(candidates, target);
            std::erase(activePatch, target);
            noVisibleTargetPosition.reset();
            breakingTarget.reset();
            breakTicks = 0;
            pendingCompletion = !continueMining && desiredQuantity > 0 && minedQuantity >= desiredQuantity;
            pendingPickupTargets.push_back(target);
            controller.stop();
            return;
        }

        if (!currentTargetState.breakable || MovementGenerator::wouldExposeLiquid(world, target)) {
            if (breakTicks > 0)
                player->getGameMode()->stopDestroyBlock({target.x, target.y, target.z});
            blacklist.insert(target);
            std::erase(candidates, target);
            std::erase(activePatch, target);
            breakingTarget.reset();
            breakTicks = 0;
            controller.stop();
            pendingMessage = "Skipped a target whose removal would release water or lava.";
            return;
        }

        const auto position = player->getPosition();
        if (distanceToBlock(position, target) > 6.10f ||
            !isVisibleFromPlayer(player, region, target)) {
            if (breakTicks > 0)
                player->getGameMode()->stopDestroyBlock({target.x, target.y, target.z});
            breakingTarget.reset();
            breakTicks = 0;
            controller.goTo(std::make_shared<GoalGetToBlock>(target));
            return;
        }
        const glm::ivec3 targetVector{target.x, target.y, target.z};
        const auto face = facingFromPlayer(position, target);
        bool destroyed = false;
        if (breakTicks == 0)
            selectBestTool(target);
        // GameMode's destroy calls update the world but do not always invoke
        // the local first-person arm animation when driven by automation.
        // Trigger the native mining swing every break tick so the visual hand
        // motion matches ordinary player mining.
        player->swing();
        if (breakTicks == 0)
            player->getGameMode()->startDestroyBlock(targetVector, face, destroyed);
        else
            player->getGameMode()->continueDestroyBlock(targetVector, face, position, destroyed);
        ++breakTicks;

        // A failed/indestructible target must not hold the whole process
        // forever. Baritone similarly blacklists implausible ore locations.
        if (breakTicks > 240) {
            player->getGameMode()->stopDestroyBlock(targetVector);
            blacklist.insert(target);
            breakingTarget.reset();
            candidates.clear();
            activePatch.clear();
            breakTicks = 0;
            controller.stop();
            pendingMessage = "Skipped an unbreakable or timed-out target.";
        }
        return;
    }

    if (controller.getState() == ControllerState::Calculating ||
        controller.getState() == ControllerState::Executing ||
        controller.getState() == ControllerState::Paused)
        return;

    if (controller.getState() == ControllerState::Arrived &&
        (!activePatch.empty() || !candidates.empty())) {
        const auto playerBlock = playerFeetBlock(player);
        const auto playerPosition = player->getPosition();
        auto& targets = activePatch.empty() ? candidates : activePatch;
        auto reachable = targets.end();
        float bestDistance = std::numeric_limits<float>::infinity();
        for (auto iterator = targets.begin(); iterator != targets.end(); ++iterator) {
            const float distance = distanceToBlock(playerPosition, *iterator);
            if (distance > 6.10f || distance >= bestDistance ||
                !matches(blockLegacyAt(region, *iterator)) ||
                !isVisibleFromPlayer(player, region, *iterator))
                continue;
            bestDistance = distance;
            reachable = iterator;
        }
        if (reachable != targets.end()) {
            noVisibleTargetPosition.reset();
            beginBreaking(*reachable);
            return;
        }

        // Clear the first visible obstruction, never mine ore through a wall.
        // A satisfied adjacency goal must either make progress or retire the
        // inaccessible patch instead of idling here forever.
        for (const auto& target : targets) {
            const glm::vec3 ray = glm::vec3{target.x + 0.5f, target.y + 0.5f, target.z + 0.5f} - playerPosition;
            const int steps = std::max(1, static_cast<int>(std::ceil(glm::length(ray) / 0.05f)));
            for (int step = 1; step < steps; ++step) {
                const auto cell = actorBlock(playerPosition + ray * (static_cast<float>(step) / steps));
                const auto state = world.getBlock(cell);
                if (!state.loaded || state.liquid || state.hazard)
                    break;
                if (!state.solid)
                    continue;
                // Keep the player's current footing intact during direct mining.
                if (state.breakable && cell != playerBlock.offset(0, -1, 0) &&
                    distanceToBlock(playerPosition, cell) <= 6.10f &&
                    !MovementGenerator::wouldExposeLiquid(world, cell) &&
                    isVisibleFromPlayer(player, region, cell)) {
                    beginBreaking(cell);
                    breakingTargetCountsGoal = fixedTargetMode
                        ? std::ranges::find(fixedTargets, cell) != fixedTargets.end()
                        : matches(blockLegacyAt(region, cell));
                    return;
                }
                break;
            }
        }
        for (const auto& target : targets)
            blacklist.insert(target);
        activePatch.clear();
        candidates.clear();
        controller.stop();
    }

    if (controller.getState() == ControllerState::Failed) {
        // A failed route may still leave a visible target in reach.
        const auto position = player->getPosition();
        auto& targets = activePatch.empty() ? candidates : activePatch;
        auto directTarget = targets.end();
        float bestDistance = std::numeric_limits<float>::infinity();
        for (auto iterator = targets.begin(); iterator != targets.end(); ++iterator) {
            if (!matches(blockLegacyAt(region, *iterator)))
                continue;
            const float distance = distanceToBlock(position, *iterator);
            if (distance > 6.10f || distance >= bestDistance ||
                !isVisibleFromPlayer(player, region, *iterator))
                continue;
            bestDistance = distance;
            directTarget = iterator;
        }
        if (directTarget != targets.end()) {
            noVisibleTargetPosition.reset();
            beginBreaking(*directTarget);
            return;
        }

        // This patch is genuinely blocked by unsafe/unbreakable terrain. Skip
        // only this connected patch; blacklisting every result from the same
        // scan made mining bounce through unrelated hallway goals.
        const auto failedTargets = activePatch.empty() ? candidates : activePatch;
        for (const auto& candidate : failedTargets)
            blacklist.insert(candidate);
        std::erase_if(candidates, [&](const auto& candidate) {
            return blacklist.contains(candidate);
        });
        activePatch.clear();
        noVisibleTargetPosition.reset();
        controller.stop();
        pendingMessage = "Nearest ore patch is blocked; trying the next closest patch.";
    }

    // Keep working on the known vein after each drop. A 64-block scan is
    // needlessly expensive between adjacent blocks and was the source of the
    // visible pause after every diamond. Only scan again when this patch is
    // exhausted or invalidated.
    std::erase_if(activePatch, [&](const auto& patchTarget) {
        return blacklist.contains(patchTarget) || !matches(blockLegacyAt(region, patchTarget));
    });
    if (activePatch.empty()) {
        candidates = scan();
        if (candidates.empty()) {
            noVisibleTargetPosition.reset();
            active = false;
            controller.stop();
            restoreHotbar();
            restoreBreakPolicy(controller);
            if (fixedTargetMode)
                pendingMessage = "Tunnel clearing complete: " + std::to_string(minedQuantity) + " block(s) removed.";
            else
                pendingMessage = "No matching loaded blocks were found within " +
                    std::to_string(scanRadius) + " blocks.";
            return;
        }

        // The scan is sorted by distance, so the first candidate is the
        // nearest block. Flood-fill all matching blocks touching it and keep
        // that patch as the active composite goal.
        activePatch = collectPatch(candidates.front(), candidates);
        if (activePatch.empty())
            activePatch = {candidates.front()};
        if (activePatch.size() > 1)
            pendingMessage = "Ore patch found: " + std::to_string(activePatch.size()) +
                " connected block(s); continuing through the patch.";
    }

    // The first scan can discover an ore that is already within interaction
    // range. Start the normal destroy lifecycle immediately instead of asking
    // A* to produce a one-step path that appears to do nothing.
    auto nearestReachable = activePatch.end();
    float nearestReachableDistance = std::numeric_limits<float>::infinity();
    for (auto iterator = activePatch.begin(); iterator != activePatch.end(); ++iterator) {
        if (!matches(blockLegacyAt(region, *iterator)) ||
            distanceToBlock(player->getPosition(), *iterator) > 6.10f ||
            !isVisibleFromPlayer(player, region, *iterator))
            continue;
        const float distance = distanceToBlock(player->getPosition(), *iterator);
        if (distance < nearestReachableDistance) {
            nearestReachableDistance = distance;
            nearestReachable = iterator;
        }
    }
    if (nearestReachable != activePatch.end()) {
        // The normal in-range shortcut still requires a visible target. An
        // occluded target gets a path attempt first, with direct wall mining
        // reserved for the failed-path fallback above.
        beginBreaking(*nearestReachable);
        return;
    }

    std::vector<std::shared_ptr<Goal>> goals;
    goals.reserve(activePatch.size());
    for (const auto& candidate : activePatch)
        goals.push_back(std::make_shared<GoalGetToBlock>(candidate));
    if (!controller.goTo(std::make_shared<GoalComposite>(std::move(goals)))) {
        active = false;
        restoreHotbar();
        restoreBreakPolicy(controller);
        pendingMessage = "Mining stopped because no world is available.";
    }
}

bool MiningProcess::isActive() const { return active; }

int MiningProcess::getMinedQuantity() const { return minedQuantity; }

int MiningProcess::getDesiredQuantity() const { return desiredQuantity; }

std::string MiningProcess::getStatusLine() const {
    if (!active)
        return "mine idle";
    std::string result = "mined " + std::to_string(minedQuantity);
    if (desiredQuantity > 0)
        result += "/" + std::to_string(desiredQuantity);
    if (breakingTarget)
        result += " | breaking " + std::to_string(breakingTarget->x) + " " +
            std::to_string(breakingTarget->y) + " " + std::to_string(breakingTarget->z);
    return result;
}

std::vector<BlockPos> MiningProcess::getRenderTargets() const {
    if (!active)
        return {};
    auto targets = activePatch;
    if (breakingTarget && std::ranges::find(targets, *breakingTarget) == targets.end())
        targets.push_back(*breakingTarget);
    return targets;
}

std::optional<std::string> MiningProcess::takeMessage() {
    return std::exchange(pendingMessage, std::nullopt);
}

bool MiningProcess::matches(BlockLegacy* block) const {
    if (block == nullptr)
        return false;
    // BlockLegacy names are already lowercase in Bedrock. Compare a view
    // directly so a 64-block scan does not allocate and lowercase a string
    // for every non-matching block.
    const int blockId = static_cast<int>(block->getBlockId());
    std::string_view name = block->getName();
    if (name.starts_with("minecraft:"))
        name.remove_prefix(10);
    const bool neverBreak = blockId == 7 || name == "bedrock" ||
        name == "invisible_bedrock" || name == "barrier" ||
        name == "structure_void" || name == "end_portal_frame" ||
        name == "reinforced_deepslate" || name == "border_block" ||
        name == "allow" || name == "deny";
    if (neverBreak)
        return false;
    if (fixedTargetMode)
        return blockId != 0;
    if (std::ranges::binary_search(blockIds, blockId))
        return true;
    if (std::ranges::binary_search(blockNames, name))
        return true;

    // Treat an ore name as a family, matching the stone and deepslate
    // variants even when a server reports an unexpected namespace/variant.
    for (const auto& requested : blockNames) {
        constexpr std::string_view deepslatePrefix = "deepslate_";
        if (requested.ends_with("_ore") && name.starts_with(deepslatePrefix) &&
            name.substr(deepslatePrefix.size()) == requested)
            return true;
    }
    return false;
}

std::vector<BlockPos> MiningProcess::scan() const {
    auto* player = MC::getLocalPlayer();
    auto* region = MC::getRegion();
    if (player == nullptr || region == nullptr)
        return {};

    const auto origin = playerFeetBlock(player);
    if (fixedTargetMode) {
        auto found = fixedTargets;
        std::erase_if(found, [&](const auto& pos) {
            return blacklist.contains(pos) || !matches(blockLegacyAt(region, pos));
        });
        std::ranges::sort(found, [&](const auto& left, const auto& right) {
            return distanceSquared(left, origin) < distanceSquared(right, origin);
        });
        if (found.size() > 48)
            found.resize(48);
        return found;
    }
    const int verticalRadius = std::min(scanRadius, 24);
    std::vector<BlockPos> found;
    found.reserve(64);

    // Search horizontal shells from the player outward. Mining only needs the
    // nearest patch; scanning the entire 64-block disk before accepting the
    // first nearby ore was the main source of the one-second hitch.
    for (int shell = 0; shell <= scanRadius && found.empty(); ++shell) {
        for (int dx = -shell; dx <= shell; ++dx) {
            for (int dz = -shell; dz <= shell; ++dz) {
                if (std::max(std::abs(dx), std::abs(dz)) != shell ||
                    dx * dx + dz * dz > scanRadius * scanRadius)
                    continue;
                for (int y = origin.y - verticalRadius; y <= origin.y + verticalRadius; ++y) {
                    const BlockPos pos{origin.x + dx, y, origin.z + dz};
                    if (blacklist.contains(pos))
                        continue;
                    if (matches(blockLegacyAt(region, pos)))
                        found.push_back(pos);
                }
            }
        }
    }

    std::ranges::sort(found, [&](const auto& left, const auto& right) {
        return distanceSquared(left, origin) < distanceSquared(right, origin);
    });
    // Keep enough nearby results to flood-fill a whole vein. The active goal
    // is still limited to one connected patch, so this does not make A* solve
    // every ore in the scan radius at once.
    if (found.size() > 128)
        found.resize(128);
    return found;
}

std::vector<BlockPos> MiningProcess::collectPatch(const BlockPos& seed,
    const std::vector<BlockPos>& source) const {
    std::vector<BlockPos> patch;
    patch.push_back(seed);
    std::unordered_set<BlockPos, BlockPosHash> included;
    included.insert(seed);

    // Fixed target clearing must stay limited to the explicit target list.
    // Normal ore mining can query every touching neighbor directly, which
    // means a shell-ordered scan can stop as soon as it sees the nearest patch
    // without losing diagonal members of that vein.
    if (!fixedTargetMode) {
        auto* region = MC::getRegion();
        if (region == nullptr)
            return patch;

        constexpr std::size_t maximumPatchSize = 128;
        for (std::size_t cursor = 0; cursor < patch.size() && patch.size() < maximumPatchSize; ++cursor) {
            const auto current = patch[cursor];
            // Treat face-, edge-, and corner-touching ore as one patch. Ore
            // generation commonly places vein members diagonally; keeping
            // only six face neighbors made those blocks get discovered and
            // mined immediately afterward without being highlighted together.
            for (int ox = -1; ox <= 1 && patch.size() < maximumPatchSize; ++ox) {
                for (int oy = -1; oy <= 1 && patch.size() < maximumPatchSize; ++oy) {
                    for (int oz = -1; oz <= 1 && patch.size() < maximumPatchSize; ++oz) {
                        if (ox == 0 && oy == 0 && oz == 0)
                            continue;
                        const auto candidate = current.offset(ox, oy, oz);
                        if (blacklist.contains(candidate) || included.contains(candidate) ||
                            !matches(blockLegacyAt(region, candidate)))
                            continue;
                        included.insert(candidate);
                        patch.push_back(candidate);
                    }
                }
            }
        }
        return patch;
    }

    // Ore veins are face-connected. Do not merge blocks that merely touch at
    // an edge/corner; those are separate pockets and should be discovered only
    // after the nearer visible pocket ends.
    for (std::size_t cursor = 0; cursor < patch.size(); ++cursor) {
        const auto& current = patch[cursor];
        for (const auto& candidate : source) {
            if (included.contains(candidate))
                continue;
            if (std::abs(candidate.x - current.x) +
                std::abs(candidate.y - current.y) +
                std::abs(candidate.z - current.z) != 1)
                continue;
            included.insert(candidate);
            patch.push_back(candidate);
        }
    }
    return patch;
}

void MiningProcess::beginBreaking(const BlockPos& target) {
    const BedrockWorld world(MC::getRegion());
    const auto state = world.getBlock(target);
    if (!state.loaded || !state.breakable || MovementGenerator::wouldExposeLiquid(world, target)) {
        blacklist.insert(target);
        std::erase(candidates, target);
        std::erase(activePatch, target);
        breakingTarget.reset();
        breakTicks = 0;
        pendingMessage = state.breakable
            ? "Skipped a target whose removal would release water or lava."
            : "Skipped an unbreakable mining target.";
        return;
    }
    breakingTarget = target;
    breakingTargetCountsGoal = true;
    breakTicks = 0;
}

void MiningProcess::selectBestTool(const BlockPos& target) {
    auto* player = MC::getLocalPlayer();
    auto* region = MC::getRegion();
    if (player == nullptr || region == nullptr || player->getSupplies() == nullptr)
        return;
    auto* inventory = player->getSupplies()->getInventory();
    auto* block = region->getBlock(target.x, target.y, target.z);
    if (inventory == nullptr || block == nullptr)
        return;

    int bestSlot = player->getSupplies()->getSelectedHotbarSlot();
    float bestSpeed = 0.f;
    for (int slot = 0; slot < 9; ++slot) {
        auto* stack = inventory->getItem(slot);
        if (stack == nullptr || !stack->isValid())
            continue;
        const float speed = stack->getDestroySpeed(block);
        if (speed > bestSpeed) {
            bestSpeed = speed;
            bestSlot = slot;
        }
    }
    if (previousHotbarSlot < 0)
        previousHotbarSlot = player->getSupplies()->getSelectedHotbarSlot();
    player->getSupplies()->setSelectedHotbarSlot(bestSlot);
}

void MiningProcess::restoreHotbar() {
    if (previousHotbarSlot < 0)
        return;
    if (auto* player = MC::getLocalPlayer(); player != nullptr && player->getSupplies() != nullptr)
        player->getSupplies()->setSelectedHotbarSlot(previousHotbarSlot);
    previousHotbarSlot = -1;
}

void MiningProcess::enableBreakPolicy(BaritoneController& controller) {
    if (ownsBreakPolicy)
        return;
    auto& options = controller.getOptions();
    previousAllowBreak = options.allowBreak;
    previousAllowWater = options.allowWater;
    previousAllowParkour = options.allowParkour;
    previousAllowParkourAscend = options.allowParkourAscend;
    previousAllowBridge = options.allowBridge;
    previousBridgeOverWaterOnly = options.bridgeOverWaterOnly;
    previousBridgeOnlyAfterFailure = options.bridgeOnlyAfterFailure;
    previousMiningMode = options.miningMode;
    previousMaxExpandedNodes = options.maxExpandedNodes;
    previousNodesPerTick = options.nodesPerTick;
    previousHeuristicWeight = options.heuristicWeight;
    options.allowBreak = true;
    // Mining routes must not enter liquid, even when ordinary navigation is
    // configured to allow swimming.
    options.allowWater = false;
    // Mining should use ordinary walk/step movement. Parkour-ascend can
    // generate a raised landing two blocks above the current feet position,
    // which looks like the miner is repeatedly jumping into a wall.
    options.allowParkour = false;
    options.allowParkourAscend = false;
    // Permit only the small cardinal water-bridge generator. It lets mining
    // cross a pool while excluding arbitrary scaffolding and the expensive
    // general bridge search that previously caused pathfinding hitches.
    options.allowBridge = true;
    options.bridgeOverWaterOnly = true;
    options.bridgeOnlyAfterFailure = false;
    options.miningMode = true;
    // Mining has a bounded local search so an unreachable loaded target cannot
    // freeze the client while exploring tens of thousands of cave nodes.
    options.maxExpandedNodes = std::min<std::size_t>(options.maxExpandedNodes, 24000);
    // Keep each game tick responsive. The pathfinder now caches block states,
    // so this lower per-tick slice still completes local routes quickly
    // without producing the large frame hitch caused by 500 expansions.
    options.nodesPerTick = std::clamp<std::size_t>(options.nodesPerTick, 120, 300);
    // Weighted A* deliberately favors forward progress during mining. Combined
    // with break transitions this makes a short safe tunnel win over a long
    // walk back through previously mined hallways.
    options.heuristicWeight = 1.75;
    ownsBreakPolicy = true;
}

void MiningProcess::restoreBreakPolicy(BaritoneController& controller) {
    if (!ownsBreakPolicy)
        return;
    auto& options = controller.getOptions();
    options.allowBreak = previousAllowBreak;
    options.allowWater = previousAllowWater;
    options.allowParkour = previousAllowParkour;
    options.allowParkourAscend = previousAllowParkourAscend;
    options.allowBridge = previousAllowBridge;
    options.bridgeOverWaterOnly = previousBridgeOverWaterOnly;
    options.bridgeOnlyAfterFailure = previousBridgeOnlyAfterFailure;
    options.miningMode = previousMiningMode;
    options.maxExpandedNodes = previousMaxExpandedNodes;
    options.nodesPerTick = previousNodesPerTick;
    options.heuristicWeight = previousHeuristicWeight;
    ownsBreakPolicy = false;
}

} // namespace baritone
