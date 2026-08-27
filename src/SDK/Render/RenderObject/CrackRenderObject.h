#pragma once

struct CrackRenderObject {
    std::shared_ptr<mce::Mesh> mesh;
    mce::MaterialPtr* crackMat;
    bool alphaTest;
};
