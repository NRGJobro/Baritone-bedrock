#pragma once

#include "../MCE/framebuilder/CustomSurfaceShaderMetadata.h"

namespace dragon {
    struct RenderMetadata {
        uint64_t id;
        mce::framebuilder::CustomSurfaceShaderMetadata* surfaceShaderMetadata;
        bool unk;
        uint64_t unk2;
        char pad[0x18];
        int unk3;
    };
}
