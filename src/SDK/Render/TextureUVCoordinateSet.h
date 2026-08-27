#pragma once

#include "../Client/Resources/ResourceLocation.h"

struct TextureUVCoordinateSet {
    float weight;
    float uMin;
    float vMin;
    float uMax;
    float vMax;
    uint16_t texWidth;
    uint16_t texHeight;
    ResourceLocation sourceFileLocation;
    uint8_t isotropicFaceData;
    int16_t textureSetTranslationIndex;
    uint16_t pbrTextureDataHandle;
};
