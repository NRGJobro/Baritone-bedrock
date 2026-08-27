#pragma once

#include "../Network/NetEventCallback.h"
#include "Actor/EntityContext/EntityContext.h"

class GameSession {
    char pad0x0[0x10]; // 0x0

public:
    EntityContext context;

private:
    char pad0x28[0x28]; // 0x28

public:
    std::unique_ptr<NetEventCallback> clientNetworkHandler; // 0x50

    [[nodiscard]] class Level* getLevel() const;
};
