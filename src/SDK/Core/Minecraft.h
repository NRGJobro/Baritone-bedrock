#pragma once

#include "../World/GameSession.h"

class Minecraft {
public:
    GameSession* getGameSession();
    std::shared_ptr<EntityRegistry> getEntityRegistry();
};
