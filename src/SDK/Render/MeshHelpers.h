#pragma once

#include "../Client/MCE/MaterialPtr.h"
#include "../Client/Texture/BedrockTextureData.h"
#include "ScreenContext.h"

class MeshHelpers {
public:
    static void renderMeshImmediately(ScreenContext* screenContext, Tessellator* tessellator, mce::MaterialPtr* material);
    static void renderMeshImmediately(ScreenContext* screenContext, Tessellator* tessellator, mce::MaterialPtr* material, const BedrockTextureData& texture);
    static void renderMeshImmediately(ScreenContext* screenContext, Tessellator* tessellator, mce::MaterialPtr* material, const mce::ClientTexture& texture);
};
