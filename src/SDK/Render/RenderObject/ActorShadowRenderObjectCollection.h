#pragma once

#include "../../Core/Memory/LinearAllocator.h"
#include "ActorShadowRenderObject.h"

struct ActorShadowRenderObjectCollection {
    const std::vector<ActorShadowRenderObject, LinearAllocator<ActorShadowRenderObject>> entityShadows;
    std::shared_ptr<mce::Mesh> shadowCylinder;
    std::shared_ptr<mce::Mesh> shadowOverlayCube;
    std::shared_ptr<mce::Mesh> unk2;
    mce::MaterialPtr* shadowVolumeFront;
    mce::MaterialPtr* shadowVolumeBack;
    mce::MaterialPtr* shadowOverlayMat;
    mce::Color shadowColor;
};
