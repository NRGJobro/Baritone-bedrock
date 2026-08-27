#pragma once

struct ParticleTypeRenderObject {
    std::shared_ptr<mce::Mesh> layerMesh;
    mce::TexturePtr layerTexture;
    bool blend;
};
