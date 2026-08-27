#pragma once

#include "FogDescription.h"
#include "../BlendStateDescription.h"
#include "../CullMode.h"
#include "../Mesh.h"
#include "../../dragon/res/ServerTexture.h"

namespace mce::framebuilder {
    struct RenderParticleDescription {
        Mesh& mesh;
        dragon::res::ServerTexture texture;
        char pad1[0x20];
        char pad2[0x20];
        dragon::res::ServerTexture unknownTexture1;
        dragon::res::ServerTexture unknownTexture2;
        uint32_t unk;
        FogDescription fog;
        float farChunkDistance;
        BlendStateDescription blendStateDescription;
        CullMode cullMode;
        uint16_t viewId;
    };
}
