#pragma once

#include "../Client/MCE/MaterialPtr.h"
#include "ScreenContext.h"

class MeshHelpers {
public:
    static void renderMeshImmediately(ScreenContext* screenContext, Tessellator* tessellator, mce::MaterialPtr* material);
};
