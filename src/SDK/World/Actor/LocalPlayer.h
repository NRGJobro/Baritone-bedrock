#pragma once

#include "Player.h"
#include "GameMode.h"

class LocalPlayer : public Player {
public:
    GameMode* getGameMode();
};
