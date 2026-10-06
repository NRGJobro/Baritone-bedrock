#pragma once

#include "../../Client/SubClientId.h"
#include "../Compressibility.h"
#include "../NetworkPeer.h"
#include "MinecraftPacketIds.h"
#include "IPacketHandlerDispatcher.h"
#include "PacketPriority.h"

class Packet {
    void** vtable;

public:
    PacketPriority priority;
    NetworkPeer::Reliability reliability;
    SubClientId senderSubId;
    bool isHandled;
    NetworkPeer::PacketRecvTimepoint receiveTimepoint;
    IPacketHandlerDispatcher* handler;
    Compressibility compressible;

    MinecraftPacketIds getID();
    std::string_view getName();
};

static_assert(sizeof(Packet) == 0x30);
