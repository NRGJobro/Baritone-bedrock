#include "ClipUtils.h"

#include "../SDK/MC.h"

bool ClipUtils::canSee(const glm::vec3& start, const int maxDistance, Actor* actor, const bool feet, const bool head) {
    if (!feet && !head)
        return true;

    glm::vec3 headPos{}, feetPos{};

    if (head)
        headPos = actor->getAttachPos(ActorLocation::Head, 0.f);

    if (feet)
        feetPos = actor->getAttachPos(ActorLocation::Feet, 0.f);

    const BlockSource::DefaultBlockScan checkBlock{};

    const auto region = MC::getLocalPlayer()->getDimension()->getBlockSource();

    if (feet && head) {
        if (glm::ivec3(headPos) == glm::ivec3(feetPos)) {
            const auto result = region->clip(start, headPos, false, ShapeType::Outline, maxDistance, true, false, nullptr, checkBlock, false);

            return result.type != HitResultType::Tile;
        }
    }

    if (head) {
        const auto result = region->clip(start, headPos, false, ShapeType::Outline, maxDistance, true, false, nullptr, checkBlock, false);

        if (result.type != HitResultType::Tile)
            return true;
    }

    if (feet) {
        const auto result = region->clip(start, feetPos, false, ShapeType::Outline, maxDistance, true, false, nullptr, checkBlock, false);

        if (result.type != HitResultType::Tile)
            return true;
    }

    return false;
}
