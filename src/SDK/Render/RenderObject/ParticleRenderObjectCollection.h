#pragma once

#include "ParticleLayerRenderObject.h"

struct ParticleRenderObjectCollection {
    ParticleLayerRenderObject alphaTested;
    ParticleLayerRenderObject blended;
    ParticleLayerRenderObject opaque;
};
