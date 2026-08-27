#pragma once

#include "CrackRenderObject.h"

struct CrackRenderObjectCollection {
    const std::vector<CrackRenderObject, LinearAllocator<CrackRenderObject>> cracks;
    mce::TexturePtr atlasTexture;
};
