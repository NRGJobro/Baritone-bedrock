#pragma once

#include "../Mesh.h"
#include "../../dragon/rendering/ClipSpaceOrigin.h"
#include "../../dragon/res/ServerTexture.h"

namespace mce::framebuilder {
    struct BlitFlipbookSingleTextureDescription {
        Mesh& mesh;
        dragon::res::ServerTexture blitTexture;
        float vBlendFrom;
        float vBlendTo;
        float vBlendAmount;
        ViewportInfo viewportInfo;
        dragon::rendering::ClipSpaceOrigin clipSpaceOrigin;
    };
}
