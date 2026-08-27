#pragma once

#include "../Client/MinecraftGame.h"
#include "ScreenContext.h"

class BaseActorRenderContext {
    char pad[0x300]{};

public:
    BaseActorRenderContext(ScreenContext* screenContext, ClientInstance* clientInstance, MinecraftGame* minecraftGame);

    class ItemRenderer* getItemRenderer();
};
