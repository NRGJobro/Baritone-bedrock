#pragma once

#include "../Core/BlockPos.h"

#include <optional>
#include <string>

namespace baritone {

class BaritoneController;

// A lightweight loaded-world explorer. It visits chunk centers in expanding
// square rings and relies on partial-path continuation to load terrain ahead.
class ExploreProcess {
    BlockPos origin{};
    int originChunkX = 0;
    int originChunkZ = 0;
    int maximumRadius = 0;
    int ring = 1;
    int perimeterIndex = 0;
    int visited = 0;
    bool active = false;
    std::optional<std::string> pendingMessage;

    [[nodiscard]] std::optional<BlockPos> nextGoal();

public:
    void start(const BlockPos& origin, int radiusChunks = 0);
    void cancel(BaritoneController& controller);
    void tick(BaritoneController& controller);

    [[nodiscard]] bool isActive() const;
    [[nodiscard]] std::string getStatusLine() const;
    std::optional<std::string> takeMessage();
};

} // namespace baritone
