#include "ExploreProcess.h"

#include "../BaritoneController.h"
#include "../Core/Goal.h"

#include <algorithm>
#include <memory>
#include <utility>

namespace baritone {

void ExploreProcess::start(const BlockPos& newOrigin, const int radiusChunks) {
    origin = newOrigin;
    originChunkX = newOrigin.x >= 0 ? newOrigin.x / 16 : (newOrigin.x - 15) / 16;
    originChunkZ = newOrigin.z >= 0 ? newOrigin.z / 16 : (newOrigin.z - 15) / 16;
    maximumRadius = std::max(0, radiusChunks);
    ring = 1;
    perimeterIndex = 0;
    visited = 0;
    active = true;
    pendingMessage = maximumRadius == 0 ? "Continuous exploration started." :
        "Exploring a " + std::to_string(maximumRadius) + "-chunk radius.";
}

void ExploreProcess::cancel(BaritoneController& controller) {
    active = false;
    controller.stop();
}

void ExploreProcess::resetForWorldChange() {
    active = false;
    origin = {};
    originChunkX = 0;
    originChunkZ = 0;
    maximumRadius = 0;
    ring = 1;
    perimeterIndex = 0;
    visited = 0;
    pendingMessage.reset();
}

void ExploreProcess::tick(BaritoneController& controller) {
    if (!active || controller.getState() == ControllerState::Calculating ||
        controller.getState() == ControllerState::Executing ||
        controller.getState() == ControllerState::Paused)
        return;

    auto goal = nextGoal();
    if (!goal) {
        active = false;
        controller.stop();
        pendingMessage = "Exploration complete after " + std::to_string(visited) + " chunks.";
        return;
    }

    ++visited;
    if (!controller.goTo(std::make_shared<GoalXZ>(goal->x, goal->z))) {
        active = false;
        pendingMessage = "Exploration stopped because no world is available.";
    }
}

bool ExploreProcess::isActive() const { return active; }

std::string ExploreProcess::getStatusLine() const {
    if (!active)
        return "explore idle";
    return "exploring ring " + std::to_string(ring) + " | chunks " + std::to_string(visited);
}

std::optional<std::string> ExploreProcess::takeMessage() {
    return std::exchange(pendingMessage, std::nullopt);
}

std::optional<BlockPos> ExploreProcess::nextGoal() {
    if (maximumRadius > 0 && ring > maximumRadius)
        return std::nullopt;

    const int sideLength = ring * 2;
    const int side = perimeterIndex / sideLength;
    const int offset = perimeterIndex % sideLength;
    int chunkX = 0;
    int chunkZ = 0;
    switch (side) {
    case 0: chunkX = -ring + offset; chunkZ = -ring; break;
    case 1: chunkX = ring; chunkZ = -ring + offset; break;
    case 2: chunkX = ring - offset; chunkZ = ring; break;
    default: chunkX = -ring; chunkZ = ring - offset; break;
    }

    if (++perimeterIndex >= ring * 8) {
        ++ring;
        perimeterIndex = 0;
    }

    return BlockPos{(originChunkX + chunkX) * 16 + 8, origin.y,
        (originChunkZ + chunkZ) * 16 + 8};
}

} // namespace baritone
