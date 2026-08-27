#pragma once

#include "BedrockTextureData.h"

class BedrockTexture {
public:
    std::shared_ptr<BedrockTextureData> texture;
    std::shared_ptr<BedrockTextureData> mersTexture;
    std::shared_ptr<BedrockTextureData> normalTexture;

    void unload();
};
