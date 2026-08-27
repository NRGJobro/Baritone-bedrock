#pragma once

#include "../Core/Pathfinder.h"

namespace baritone {

class Goal;
struct RenderOptions;

class PathRenderer {
public:
    static void render(const std::vector<PathNode>& path, std::size_t currentIndex,
        const std::vector<PathNode>& bestPath, const std::vector<PathNode>& recentPath,
        const Goal* goal, const RenderOptions& options);
};

} // namespace baritone
