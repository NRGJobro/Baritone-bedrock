#pragma once

#include "IEntityComponent.h"
#include "../Path/Path.h"
#include "../Path/PathNavigation.h"

class NavigationComponent : public IEntityComponent {
public:
    bool avoidDamageBlocks : 1;
    bool avoidPortals : 1;
    bool avoidSun : 1;
    bool avoidWater : 1;
    bool canBreach : 1;
    bool canFloat : 1;
    bool canPathOverLava : 1;
    bool canWalkInLava : 1;
    bool canJump : 1;
    bool canOpenDoors : 1;
    bool canOpenIronDoors : 1;
    bool canPassDoors : 1;
    bool canSink : 1;
    bool isAmphibious : 1;
    bool isFollowingRivers : 1;
    bool hasEndPathRadius : 1;
    bool hasDestination : 1;
    int tick;
    int tickTimeout;
    int lastStuckCheck;
    float endPathRadiusSqr;
    float speed;
    float terminationThreshold;
    glm::vec3 lastStuckCheckPosition;
    glm::vec3 targetOffset;

private:
    char pad[0x18];
    //std::vector<BlockDescriptor> blocksToAvoid;

public:
    std::unique_ptr<PathNavigation> navigation;
    std::unique_ptr<Path> path;
};
