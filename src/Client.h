#pragma once

#include "Utils/TimeUtils.h"

class Client {
public:
    bool running = true;
    bool clickGuiOpened = false;
    // Updated by the UI render hook. Automation may only synthesize gameplay
    // input while the HUD is the active screen.
    std::atomic_bool gameplayInputAllowed{false};
    std::atomic_bool hudScreenActive{false};
    // Lifecycle gates used by hooks during world transitions and DLL teardown.
    // Hook callbacks may execute on several Minecraft threads, so these are
    // atomic even though the normal client state is game-thread owned.
    std::atomic_bool modulesReady{false};
    std::atomic_bool shuttingDown{false};
    double deltaTime = 1.0 / 60.0;

    std::bitset<256> keys;
    std::bitset<256> blockedKeys;

    std::vector<millis> frames;
    int fps;

};

extern Client g_Client;
