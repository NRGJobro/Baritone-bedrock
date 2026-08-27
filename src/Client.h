#pragma once

#include "Utils/TimeUtils.h"

class Client {
public:
    bool running = true;
    bool clickGuiOpened = false;
    double deltaTime = 1.0 / 60.0;

    std::bitset<256> keys;
    std::bitset<256> blockedKeys;

    std::vector<millis> frames;
    int fps;

};

extern Client g_Client;
