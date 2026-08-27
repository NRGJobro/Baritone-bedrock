#pragma once

#include "ActorShadowRenderObjectCollection.h"
#include "ChunkRenderObjectCollection.h"
#include "ClientRenderData.h"
#include "CloudRenderObject.h"
#include "CrackRenderObjectCollection.h"
#include "NameTagRenderObjectCollection.h"
#include "ParticleRenderObjectCollection.h"
#include "SkyRenderObject.h"
#include "ViewRenderData.h"
#include "WeatherRenderObject.h"

struct ViewRenderObject {
    ViewRenderData viewData;
    ClientRenderData clientData;
    CloudRenderObject cloudState;
    ChunkRenderObjectCollection chunksState;
    ActorShadowRenderObjectCollection entityShadowsState;
    ParticleRenderObjectCollection particleState;
    SkyRenderObject skyState;
    WeatherRenderObject weatherState;
    CrackRenderObjectCollection crackState;
    NameTagRenderObjectCollection nameTagState;
};
