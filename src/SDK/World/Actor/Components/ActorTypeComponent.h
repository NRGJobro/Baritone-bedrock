#pragma once

#include "IEntityComponent.h"

// Bedrock's runtime actor type. Item actors use the stable vanilla type ID
// 64; experience orbs use 69. Keeping the raw value avoids pulling the large
// actor-type enum into the pathing code.
struct ActorTypeComponent : IEntityComponent {
    int type = 0;
};
