#pragma once

#include "../cg/IsMissingTexture.h"
#include "../cg/TextureLoadState.h"
#include "../cg/TextureSetImageDescription.h"
#include "../mce/ClientTexture.h"
#include "../mce/TextureDescription.h"

class BedrockTextureData {
public:
    mce::ClientTexture clientTexture;
    mce::TextureDescription textureDescription;
    IsMissingTexture isMissingTexture;
    TextureLoadState textureLoadState;
    cg::TextureSetImageDescription textureSetDescription;

    void unload();
};
