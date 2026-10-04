#pragma once

#include "../EntityContext/EntityContext.h"
#include "IEntityComponent.h"

struct ActorOwnerComponent : IEntityComponent {
    class Actor* actor;
};
