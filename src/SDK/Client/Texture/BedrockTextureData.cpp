#include "BedrockTextureData.h"

void BedrockTextureData::unload() {
    this->isMissingTexture = IsMissingTexture::No;
    *reinterpret_cast<uint8_t*>(&this->textureLoadState) &= ~2;
}
