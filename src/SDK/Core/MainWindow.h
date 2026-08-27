#pragma once

#include "HIDController.h"
#include "../Client/MinecraftGame.h"

class MainWindow {
public:
    MinecraftGame* getMinecraftGame();
    HIDController* getHIDController();
};
