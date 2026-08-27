#pragma once

struct CloudRenderObject {
    std::shared_ptr<mce::Mesh> cloudsMesh;
    mce::MaterialPtr* cloudMaterial;
    mce::Color cloudColor;
    glm::vec3 cloudDiff;
    float yTranslation;
    bool renderFog;
    int horizontalOffset;
    bool renderClouds;
};
