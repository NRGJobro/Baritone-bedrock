#include "MeshHelpers.h"

void MeshHelpers::renderMeshImmediately(ScreenContext* screenContext, Tessellator* tessellator, mce::MaterialPtr* material) {
    if (tessellator->isTessellating()) {
        mce::Mesh mesh;
        tessellator->end(mesh);
        mesh.renderMesh(screenContext, material);
        tessellator->clear();
    }
}

void MeshHelpers::renderMeshImmediately(ScreenContext* screenContext, Tessellator* tessellator, mce::MaterialPtr* material, const BedrockTextureData& texture) {
    renderMeshImmediately(screenContext, tessellator, material, texture.clientTexture);
}

void MeshHelpers::renderMeshImmediately(ScreenContext* screenContext, Tessellator* tessellator, mce::MaterialPtr* material, const mce::ClientTexture& texture) {
    if (tessellator->isTessellating()) {
        mce::Mesh mesh;
        tessellator->end(mesh);
        mesh.renderMesh(screenContext, material, texture);
        tessellator->clear();
    }
}
