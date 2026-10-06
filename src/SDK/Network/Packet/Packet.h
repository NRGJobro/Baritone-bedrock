#pragma once

#include "MinecraftPacketIds.h"

class IPacketHandlerDispatcher;

class Packet {
    void** vtable;

public:
    std::int32_t priority;
    std::int32_t reliability;
    std::uint8_t senderSubId;
    bool isHandled;
    std::chrono::steady_clock::time_point receiveTimepoint;
    IPacketHandlerDispatcher* handler;
    std::int32_t compressible;

    MinecraftPacketIds getID();
};

static_assert(sizeof(Packet) == 0x30);
