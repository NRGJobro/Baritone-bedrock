#pragma once

#include "../../World/Actor/Actor.h"

class DataDrivenRendererV2 {
public:
    struct ActorData {
        uint64_t data;
        Actor* actor;
    };

    std::vector<ActorData>& getActors();
};
