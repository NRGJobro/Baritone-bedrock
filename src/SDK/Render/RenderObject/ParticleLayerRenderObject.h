#pragma once

#include "ParticleTypeRenderObject.h"

struct ParticleLayerRenderObject {
    const std::vector<ParticleTypeRenderObject, LinearAllocator<ParticleTypeRenderObject>> layers;
    mce::MaterialPtr* layerMaterial;
};
