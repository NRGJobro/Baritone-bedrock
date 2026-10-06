#include "MeshHelpers.h"
#include "Tessellator.h"

void MeshHelpers::renderMeshImmediately(ScreenContext* screenContext, Tessellator* tessellator, mce::MaterialPtr* material) {
    if (screenContext == nullptr || tessellator == nullptr || material == nullptr)
        return;

    if (tessellator->isTessellating()) {
        mce::Mesh mesh;
        tessellator->end(mesh);
        mesh.renderMesh(screenContext, material);
        tessellator->clear();
    }
}
