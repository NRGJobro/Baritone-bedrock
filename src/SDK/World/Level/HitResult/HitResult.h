#pragma once

#include "../../../Util/AABB.h"
#include "../../Actor/EntityContext/EntityRefs.h"
#include "FacingID.h"
#include "HitResultType.h"

struct HitResult {
    glm::vec3 startPos;
    glm::vec3 rayDir;
    HitResultType type;
    FacingID facing;
    glm::ivec3 blockPos;
    glm::vec3 pos;
    WeakEntityRef entity;
    AABB entityAABB;
    bool isHitLiquid;
    FacingID liquidFacing;
    glm::ivec3 liquid;
    glm::vec3 liquidPos;
    bool indirectHit;
};
