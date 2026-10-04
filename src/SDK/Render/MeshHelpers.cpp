#include "MeshHelpers.h"

void MeshHelpers::renderMeshImmediately(ScreenContext* screenContext, Tessellator* tessellator, mce::MaterialPtr* material) {
    if (tessellator->isTessellating()) {
        mce::Mesh mesh;
        tessellator->end(mesh);
        mesh.renderMesh(screenContext, material);
        tessellator->clear();
    }
}
