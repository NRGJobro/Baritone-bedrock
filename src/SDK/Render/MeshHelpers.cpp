#include "MeshHelpers.h"
#include "Tessellator.h"

void MeshHelpers::renderMeshImmediately(ScreenContext* screenContext, Tessellator* tessellator, mce::MaterialPtr* material) {
    if (tessellator == nullptr)
        return;

    if (screenContext == nullptr || material == nullptr) {
        if (tessellator->isTessellating())
            tessellator->clear();
        return;
    }

    if (tessellator->isTessellating()) {
        mce::Mesh mesh;
        tessellator->end(mesh);
        mesh.renderMesh(screenContext, material);
        tessellator->clear();
    }
}
