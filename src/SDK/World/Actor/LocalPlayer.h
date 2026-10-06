#pragma once

#include "Player.h"

class GameMode;

class LocalPlayer : public Player {
public:
    GameMode* getGameMode();
};
