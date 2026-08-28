#pragma once

#include "IEntityComponent.h"

struct MobBodyRotationComponent : IEntityComponent {
    float bodyRotation;
    float previousBodyRotation;
};
