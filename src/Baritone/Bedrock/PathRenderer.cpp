#include "PathRenderer.h"

#include "../BaritoneController.h"
#include "../Core/AdvancedGoals.h"
#include "BedrockWorld.h"
#include "../../SDK/MC.h"
#include "../../Utils/DrawUtils.h"
#include "../../Utils/LimiterTess.h"
#include "../../Utils/TimeUtils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <numbers>
#include <unordered_set>
#include <vector>

namespace {

glm::vec3 center(const baritone::BlockPos& pos, const float offset) {
    return {pos.x + 0.5f, pos.y + offset, pos.z + 0.5f};
}

void limiterSegment(const baritone::BlockPos& from, const baritone::BlockPos& to, const bool throughWalls) {
    const glm::vec3 a = center(from, 0.50f);
    const glm::vec3 b = center(to, 0.50f);
    const glm::vec3 aTop{a.x, a.y + 0.035f, a.z};
    const glm::vec3 bTop{b.x, b.y + 0.035f, b.z};
    LimiterTess::drawLine3D(a, b, throughWalls);
    LimiterTess::drawLine3D(b, bTop, throughWalls);
    LimiterTess::drawLine3D(bTop, aTop, throughWalls);
    LimiterTess::drawLine3D(aTop, a, throughWalls);
}

void limiterHorizontalQuad(const baritone::BlockPos& pos, const float y, const bool throughWalls) {
    constexpr float inset = 0.002f;
    const float minX = pos.x + inset;
    const float maxX = pos.x + 1.f - inset;
    const float minZ = pos.z + inset;
    const float maxZ = pos.z + 1.f - inset;
    LimiterTess::drawLine3D({minX, y, minZ}, {maxX, y, minZ}, throughWalls);
    LimiterTess::drawLine3D({maxX, y, minZ}, {maxX, y, maxZ}, throughWalls);
    LimiterTess::drawLine3D({maxX, y, maxZ}, {minX, y, maxZ}, throughWalls);
    LimiterTess::drawLine3D({minX, y, maxZ}, {minX, y, minZ}, throughWalls);
}

void limiterGoal(const baritone::BlockPos& pos, const bool throughWalls, const bool animated) {
    LimiterTess::setColor({35, 255, 80, 235});
    constexpr float inset = 0.002f;
    LimiterTess::drawBox3D({pos.x + inset, static_cast<float>(pos.y), pos.z + inset},
        {pos.x + 1.f - inset, pos.y + 2.f, pos.z + 1.f - inset}, throughWalls);

    const float pulse = animated
        ? std::cos(static_cast<float>(TimeUtils::currentTimeMillis() % 2000) / 2000.f * 2.f * std::numbers::pi_v<float>)
        : 0.999f;
    limiterHorizontalQuad(pos, pos.y + 1.f + pulse, throughWalls);
    limiterHorizontalQuad(pos, pos.y + 1.f - pulse, throughWalls);
}

void limiterMiningTarget(const baritone::BlockPos& pos, const bool throughWalls, const bool animated) {
    const float phase = animated
        ? static_cast<float>(TimeUtils::currentTimeMillis() % 1600) / 1600.f * 2.f *
            std::numbers::pi_v<float>
        : 0.f;
    const float inset = 0.035f + 0.025f * (0.5f + 0.5f * std::sin(phase));
    const glm::vec3 lower{pos.x + inset, pos.y + inset, pos.z + inset};
    const glm::vec3 upper{pos.x + 1.f - inset, pos.y + 1.f - inset, pos.z + 1.f - inset};

    // Match the spectral-arrow style: a faint cyan volume makes the target
    // readable through the block while the outline stays crisp and thin.
    LimiterTess::setColor({0, 115, 135, 72});
    LimiterTess::drawFilledBox3D(lower, upper, throughWalls);
    LimiterTess::setColor({125, 245, 255, 235});
    LimiterTess::drawBox3D(lower, upper, throughWalls);
}

void limiterMiningPatch(const std::vector<baritone::BlockPos>& positions,
    const bool throughWalls, const bool animated) {
    if (positions.empty())
        return;
    if (positions.size() == 1) {
        limiterMiningTarget(positions.front(), throughWalls, animated);
        return;
    }

    std::unordered_set<baritone::BlockPos, baritone::BlockPosHash> patch;
    patch.reserve(positions.size());
    for (const auto& pos : positions)
        patch.insert(pos);

    // Draw only exposed voxel faces. Shared faces between adjacent target
    // blocks are omitted, so the result is one connected shell following the
    // real ore patch rather than a bounding box that encloses unrelated rock.
    // Faces are offset along their normal (rather than inset on every axis),
    // keeping neighboring target faces joined with no visible gaps.
    constexpr float surfaceOffset = 0.0025f;
    std::vector<std::array<glm::vec3, 4>> fillFaces;
    fillFaces.reserve(positions.size() * 3);

    struct PointKey {
        int x;
        int y;
        int z;
        auto operator<=>(const PointKey&) const = default;
    };
    struct EdgeKey {
        PointKey a;
        PointKey b;
        auto operator<=>(const EdgeKey&) const = default;
    };
    struct EdgeInfo {
        int faceMask = 0;
        int count = 0;
    };
    std::map<EdgeKey, EdgeInfo> outlineEdges;
    const auto pointKey = [](const glm::vec3& point) {
        constexpr float scale = 10000.f;
        return PointKey{static_cast<int>(std::lround(point.x * scale)),
            static_cast<int>(std::lround(point.y * scale)),
            static_cast<int>(std::lround(point.z * scale))};
    };
    const auto addEdge = [&](const glm::vec3& first, const glm::vec3& second,
                                 const int face) {
        PointKey a = pointKey(first);
        PointKey b = pointKey(second);
        if (b < a)
            std::swap(a, b);
        auto& edge = outlineEdges[EdgeKey{a, b}];
        edge.faceMask |= 1 << face;
        ++edge.count;
    };

    const auto addFace = [&](const baritone::BlockPos& pos, const int face) {
        const float x0 = static_cast<float>(pos.x);
        const float x1 = x0 + 1.f;
        const float y0 = static_cast<float>(pos.y);
        const float y1 = y0 + 1.f;
        const float z0 = static_cast<float>(pos.z);
        const float z1 = z0 + 1.f;
        glm::vec3 a{}, b{}, c{}, d{};
        switch (face) {
        case 0: a = {x0 - surfaceOffset, y0, z0}; b = {x0 - surfaceOffset, y1, z0}; c = {x0 - surfaceOffset, y1, z1}; d = {x0 - surfaceOffset, y0, z1}; break;
        case 1: a = {x1 + surfaceOffset, y0, z0}; b = {x1 + surfaceOffset, y0, z1}; c = {x1 + surfaceOffset, y1, z1}; d = {x1 + surfaceOffset, y1, z0}; break;
        case 2: a = {x0, y0 - surfaceOffset, z0}; b = {x0, y0 - surfaceOffset, z1}; c = {x1, y0 - surfaceOffset, z1}; d = {x1, y0 - surfaceOffset, z0}; break;
        case 3: a = {x0, y1 + surfaceOffset, z0}; b = {x1, y1 + surfaceOffset, z0}; c = {x1, y1 + surfaceOffset, z1}; d = {x0, y1 + surfaceOffset, z1}; break;
        case 4: a = {x0, y0, z0 - surfaceOffset}; b = {x1, y0, z0 - surfaceOffset}; c = {x1, y1, z0 - surfaceOffset}; d = {x0, y1, z0 - surfaceOffset}; break;
        default: a = {x0, y0, z1 + surfaceOffset}; b = {x0, y1, z1 + surfaceOffset}; c = {x1, y1, z1 + surfaceOffset}; d = {x1, y0, z1 + surfaceOffset}; break;
        }
        fillFaces.push_back({a, b, c, d});
        addEdge(a, b, face);
        addEdge(b, c, face);
        addEdge(c, d, face);
        addEdge(d, a, face);
    };
    for (const auto& pos : positions) {
        if (!patch.contains(pos.offset(-1, 0, 0))) addFace(pos, 0);
        if (!patch.contains(pos.offset(1, 0, 0))) addFace(pos, 1);
        if (!patch.contains(pos.offset(0, -1, 0))) addFace(pos, 2);
        if (!patch.contains(pos.offset(0, 1, 0))) addFace(pos, 3);
        if (!patch.contains(pos.offset(0, 0, -1))) addFace(pos, 4);
        if (!patch.contains(pos.offset(0, 0, 1))) addFace(pos, 5);
    }

    LimiterTess::setColor({0, 115, 135, 72});
    LimiterTess::drawFilledQuads3D(fillFaces, throughWalls);

    std::vector<std::pair<glm::vec3, glm::vec3>> edges;
    edges.reserve(outlineEdges.size());
    for (const auto& [key, info] : outlineEdges) {
        // If two adjacent blocks expose the same coplanar face, that shared
        // edge is an internal seam and should not be drawn. Perpendicular
        // face pairs remain visible as the true outer/concave silhouette.
        if (info.count > 1 && (info.faceMask & (info.faceMask - 1)) == 0)
            continue;
        const auto toVec = [](const PointKey& point) {
            constexpr float scale = 10000.f;
            return glm::vec3{point.x / scale, point.y / scale, point.z / scale};
        };
        edges.emplace_back(toVec(key.a), toVec(key.b));
    }
    LimiterTess::setColor({125, 245, 255, 235});
    LimiterTess::drawLineList3D(edges, throughWalls);
}

void renderGoal(const baritone::Goal* goal, const bool throughWalls, const bool animated) {
    if (const auto composite = dynamic_cast<const baritone::GoalComposite*>(goal)) {
        std::vector<baritone::BlockPos> miningTargets;
        for (const auto& child : composite->getGoals()) {
            if (const auto miningTarget = dynamic_cast<const baritone::GoalGetToBlock*>(child.get()))
                miningTargets.push_back(miningTarget->getTarget());
            else
                renderGoal(child.get(), throughWalls, animated);
        }
        limiterMiningPatch(miningTargets, throughWalls, animated);
    } else if (const auto block = dynamic_cast<const baritone::GoalBlock*>(goal)) {
        limiterGoal(block->getTarget(), throughWalls, animated);
    } else if (const auto interact = dynamic_cast<const baritone::GoalGetToBlock*>(goal)) {
        limiterMiningTarget(interact->getTarget(), throughWalls, animated);
    } else if (const auto twoBlocks = dynamic_cast<const baritone::GoalTwoBlocks*>(goal)) {
        limiterGoal(twoBlocks->getTarget(), throughWalls, animated);
    } else if (const auto nearGoal = dynamic_cast<const baritone::GoalNear*>(goal)) {
        limiterGoal(nearGoal->getTarget(), throughWalls, animated);
    } else if (const auto xzGoal = dynamic_cast<const baritone::GoalXZ*>(goal)) {
        LimiterTess::setColor({35, 255, 80, 235});
        constexpr float inset = 0.002f;
        LimiterTess::drawBox3D({xzGoal->getX() + inset, -64.f, xzGoal->getZ() + inset},
            {xzGoal->getX() + 1.f - inset, 320.f, xzGoal->getZ() + 1.f - inset}, throughWalls);
    } else if (const auto yGoal = dynamic_cast<const baritone::GoalYLevel*>(goal)) {
        if (const auto player = MC::getLocalPlayer()) {
            const auto feet = player->getFeetPosition();
            limiterGoal({static_cast<int>(std::floor(feet.x)), yGoal->getY(), static_cast<int>(std::floor(feet.z))},
                throughWalls, animated);
        }
    }
}

void limiterRoute(const std::vector<baritone::PathNode>& path, const std::size_t begin,
    const bool throughWalls, const mce::Color& color) {
    if (path.size() < 2 || begin + 1 >= path.size())
        return;
    LimiterTess::setColor(color);
    // Limiter combines consecutive edges with identical direction into a
    // single rendered rail. Besides matching its visuals, this removes the
    // vertical seam at every ordinary one-block path node.
    for (std::size_t i = begin; i + 1 < path.size();) {
        std::size_t next = i + 1;
        const int dx = path[next].pos.x - path[i].pos.x;
        const int dy = path[next].pos.y - path[i].pos.y;
        const int dz = path[next].pos.z - path[i].pos.z;
        while (next + 1 < path.size() &&
            path[next + 1].pos.x - path[next].pos.x == dx &&
            path[next + 1].pos.y - path[next].pos.y == dy &&
            path[next + 1].pos.z - path[next].pos.z == dz)
            ++next;
        limiterSegment(path[i].pos, path[next].pos, throughWalls);
        i = next;
    }
}

void limiterCalculationNodes(const std::vector<baritone::PathNode>& path, const bool throughWalls,
    const mce::Color& color, const float phase) {
    if (path.empty()) return;
    const float pulse = 0.55f + 0.45f * std::sin(phase);
    LimiterTess::setColor({color.r, color.g, color.b, color.a * pulse});
    for (const auto& node : path) {
        const float radius = node.movement == baritone::MovementType::Bridge ? 0.16f : 0.095f;
        const glm::vec3 c = center(node.pos, 0.52f);
        LimiterTess::drawBox3D({c.x - radius, c.y - radius, c.z - radius},
            {c.x + radius, c.y + radius, c.z + radius}, throughWalls);
    }
}

void limiterMiningBreakBlocks(const std::vector<baritone::PathNode>& path,
    const std::size_t begin, const bool throughWalls, const bool animated) {
    if (path.size() < 2 || begin >= path.size() || MC::getRegion() == nullptr)
        return;

    std::vector<baritone::BlockPos> cells;
    cells.reserve(path.size() * 3);
    const auto addCell = [&](const baritone::BlockPos& pos) {
        if (std::ranges::find(cells, pos) == cells.end())
            cells.push_back(pos);
    };
    const auto addColumn = [&](const baritone::BlockPos& feet) {
        addCell(feet);
        addCell(feet.offset(0, 1, 0));
    };

    // Mirror the executor's planned clearance cells for break movements. The
    // overlay therefore shows the actual solid blocks that will be removed,
    // rather than drawing every solid floor block beneath the route.
    for (std::size_t index = std::max<std::size_t>(1, begin); index < path.size(); ++index) {
        const auto& node = path[index];
        if (index == 0)
            continue;
        const auto& source = path[index - 1].pos;
        const int dx = std::clamp(node.pos.x - source.x, -1, 1);
        const int dz = std::clamp(node.pos.z - source.z, -1, 1);
        if (node.movement == baritone::MovementType::BreakTraverse)
            addColumn(node.pos);
        else if (node.movement == baritone::MovementType::BreakAscend) {
            addCell(source.offset(0, 2, 0));
            addColumn(node.pos);
        } else if (node.movement == baritone::MovementType::BreakDescend) {
            addColumn(source.offset(dx, 0, dz));
            addColumn(node.pos);
        } else if (node.movement == baritone::MovementType::BreakDown) {
            // Clear a stale/encroaching overhead block before opening the
            // floor. This matters in leaf canopies where the actor can settle
            // into a partially occupied source column after a short drop.
            addCell(source.offset(0, 1, 0));
            addCell(node.pos);
        }
    }

    const baritone::BedrockWorld world(MC::getRegion());
    std::vector<baritone::BlockPos> breakBlocks;
    breakBlocks.reserve(cells.size());
    for (const auto& cell : cells) {
        const auto state = world.getBlock(cell);
        if (state.loaded && state.solid && state.breakable)
            breakBlocks.push_back(cell);
    }
    if (breakBlocks.empty())
        return;

    const float phase = animated
        ? static_cast<float>(TimeUtils::currentTimeMillis() % 1200) / 1200.f *
            2.f * std::numbers::pi_v<float>
        : 0.f;
    const float alpha = animated ? 0.82f + 0.14f * (0.5f + 0.5f * std::sin(phase)) : 0.92f;
    constexpr float inset = 0.012f;
    // Keep this overlay outline-only so the world remains fully visible.
    LimiterTess::setColor(mce::Color(255, 65, 65,
        static_cast<int>(std::clamp(alpha * 255.f, 0.f, 255.f))));
    for (const auto& block : breakBlocks) {
        LimiterTess::drawBox3D(
            {block.x + inset, block.y + inset, block.z + inset},
            {block.x + 1.f - inset, block.y + 1.f - inset, block.z + 1.f - inset},
            throughWalls);
    }
}

} // namespace

namespace baritone {

void PathRenderer::render(const std::vector<PathNode>& path, const std::size_t currentIndex,
    const std::vector<PathNode>& bestPath, const std::vector<PathNode>& recentPath,
    const Goal* goal, const std::vector<BlockPos>& miningTargets,
    const bool miningActive, const RenderOptions& options) {
    if ((!options.renderPath && !options.renderGoal) || MC::getLevelRenderer() == nullptr ||
        DrawUtils::getTessellator() == nullptr || DrawUtils::getScreenContext() == nullptr)
        return;

    const bool throughWalls = options.renderThroughWalls;
    if (options.renderGoal) {
        // The mining process is authoritative for its complete live patch.
        // Controller goals can lag during direct in-range breaking and omit
        // corner-connected ore, which made a diamond get mined without ever
        // receiving the cyan target highlight.
        if (miningActive) {
            if (!miningTargets.empty())
                limiterMiningPatch(miningTargets, throughWalls, options.animatedGoal);
        } else if (goal != nullptr) {
            renderGoal(goal, throughWalls, options.animatedGoal);
        }
    }

    if (options.renderPath && path.size() >= 2) {
        // Match Baritone's PathRenderer: retain three completed positions
        // behind the executor instead of erasing the line at the active edge.
        // This also prevents predictive movement look-ahead from making the
        // rendered path appear several blocks ahead of the player.
        const std::size_t firstSegment = currentIndex > 3 ? currentIndex - 3 : 0;
        // Baritone's default colorCurrentPath is solid red; progress is shown
        // by trimming only the portion more than three positions behind.
        limiterRoute(path, firstSegment, throughWalls, {255, 0, 0, 235});
        limiterMiningBreakBlocks(path, firstSegment, throughWalls, options.animatedGoal);
    }

    // Calculation candidates are useful before a route is available, but
    // drawing them over an active route produces flickering blue/cyan nodes.
    // Once executing, the authoritative red path is the only route shown.
    if (options.renderPath && options.renderCalculations && path.empty()) {
        const float phase = static_cast<float>(TimeUtils::currentTimeMillis() % 1400) / 1400.f *
            2.f * std::numbers::pi_v<float>;
        limiterRoute(bestPath, 0, throughWalls, {0.16f, 0.39f, 1.f, 0.75f});
        limiterRoute(recentPath, 0, throughWalls, {0.12f, 0.92f, 1.f, 0.75f});
        limiterCalculationNodes(bestPath, throughWalls, {0.16f, 0.39f, 1.f, 0.85f}, phase);
        limiterCalculationNodes(recentPath, throughWalls, {0.12f, 0.92f, 1.f, 0.85f}, phase + 1.7f);
    }

    DrawUtils::setShaderColor();
}

} // namespace baritone
