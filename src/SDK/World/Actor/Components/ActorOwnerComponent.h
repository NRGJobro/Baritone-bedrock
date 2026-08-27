#pragma once

#include "../Actor.h"
#include "IEntityComponent.h"

class ActorOwnerComponent : public IEntityComponent {
public:
    std::unique_ptr<Actor> entity;
};
