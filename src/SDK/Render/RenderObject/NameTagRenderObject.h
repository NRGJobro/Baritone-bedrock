#pragma once

struct NameTagRenderObject {
    std::string nameTag;
    std::shared_ptr<mce::Mesh> mesh;
    mce::MaterialPtr* tagMat;
    mce::MaterialPtr* textMatOverride;
    mce::Color tagColor;
    mce::Color textColor;
    glm::vec3 pos;
};
