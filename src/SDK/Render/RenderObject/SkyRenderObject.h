#pragma once

#include "../../Client/mce/ServerTexture.h"
#include "../../Core/Memory/LinearAllocator.h"

struct SkyRenderObject {
    std::shared_ptr<mce::Mesh> skyMesh;
    std::shared_ptr<mce::Mesh> starsMesh;
    std::shared_ptr<mce::Mesh> sunMesh;
    std::shared_ptr<mce::Mesh> moonMesh;
    mce::TexturePtr endSkyTex;
    mce::TexturePtr sunTex;
    mce::TexturePtr moonTex;
    std::vector<mce::TexturePtr, LinearAllocator<mce::TexturePtr>> cubemapTextures;
    mce::ServerTexture cubemapTexture;
    mce::MaterialPtr* cubemapMaterial;
    mce::MaterialPtr* skyMaterial;
    mce::MaterialPtr* starsMaterial;
    mce::MaterialPtr* sunMoonMaterial;
    float starBrightness;
    float sunAngleOne;
    float sunAngleA;
    float fogLevel;
    float ambientBrightness;
    float skyDarken;
};
