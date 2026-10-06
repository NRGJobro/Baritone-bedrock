#pragma once

#include "../../../Util/AABB.h"
#include "FacingID.h"
#include "HitResultType.h"

struct HitResult {
    glm::vec3 startPos;
    glm::vec3 rayDir;
    HitResultType type;
    FacingID facing;
    glm::ivec3 blockPos;
    glm::vec3 pos;
    // Native WeakEntityRef occupies 0x18 bytes (weak_ptr storage + entity id).
    // Limiter never dereferences it; preserving the footprint keeps the
    // following HitResult fields at their native offsets without retaining
    // the unused entity-reference SDK hierarchy.
    alignas(8) std::byte entity[0x18]{};
    AABB entityAABB;
    bool isHitLiquid;
    FacingID liquidFacing;
    glm::ivec3 liquid;
    glm::vec3 liquidPos;
    bool indirectHit;
};
