#pragma once

#include "../../Client/mce/Color.h"

struct ViewRenderData {
    glm::vec3 cameraPos;
    glm::vec3 cameraTargetPos;
    mce::Color fogColor;
    mce::Color skyColor;
    mce::Color sunriseColor;
    bool isEndDimension;
    float fakeHDR;
    float minParticleDistance;
    float renderDistance;
    float skyBrightnessScalar;
    bool cameraAboveClouds;
    bool cameraUnderLiquid;
    bool drawClouds;
    bool drawEntityEffects;
    bool drawInsideCubes;
    bool drawNameTags;
    bool drawParticles;
    bool isFancyRendering;
    bool drawSky;
    bool drawVRCursorInWorld;
    bool drawVRHitFlash;
    bool drawWeather;
    bool isShadowPass;
    bool showChunkMap;
};
