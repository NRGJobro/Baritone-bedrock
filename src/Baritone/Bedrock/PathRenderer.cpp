#include "PathRenderer.h"

#include "../BaritoneController.h"
#include "../Core/AdvancedGoals.h"
#include "../../SDK/MC.h"
#include "../../Utils/DrawUtils.h"
#include "../../Utils/LimiterTess.h"
#include "../../Utils/TimeUtils.h"

#include <cmath>
#include <numbers>

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

void renderGoal(const baritone::Goal* goal, const bool throughWalls, const bool animated) {
    if (const auto block = dynamic_cast<const baritone::GoalBlock*>(goal)) {
        limiterGoal(block->getTarget(), throughWalls, animated);
    } else if (const auto interact = dynamic_cast<const baritone::GoalGetToBlock*>(goal)) {
        limiterGoal(interact->getTarget(), throughWalls, animated);
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

} // namespace

namespace baritone {

void PathRenderer::render(const std::vector<PathNode>& path, const std::size_t currentIndex,
    const std::vector<PathNode>& bestPath, const std::vector<PathNode>& recentPath,
    const Goal* goal, const RenderOptions& options) {
    if ((!options.renderPath && !options.renderGoal) || MC::getLevelRenderer() == nullptr ||
        DrawUtils::getTessellator() == nullptr || DrawUtils::getScreenContext() == nullptr)
        return;

    const bool throughWalls = options.renderThroughWalls;
    if (options.renderGoal && goal != nullptr)
        renderGoal(goal, throughWalls, options.animatedGoal);

    if (options.renderPath && path.size() >= 2) {
        // Match Baritone's PathRenderer: retain three completed positions
        // behind the executor instead of erasing the line at the active edge.
        // This also prevents predictive movement look-ahead from making the
        // rendered path appear several blocks ahead of the player.
        const std::size_t firstSegment = currentIndex > 3 ? currentIndex - 3 : 0;
        // Baritone's default colorCurrentPath is solid red; progress is shown
        // by trimming only the portion more than three positions behind.
        limiterRoute(path, firstSegment, throughWalls, {255, 0, 0, 235});
    }

    if (options.renderPath && options.renderCalculations) {
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
