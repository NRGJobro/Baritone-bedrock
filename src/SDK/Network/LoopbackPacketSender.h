#pragma once

#include "Packet/Packet.h"

class LoopbackPacketSender {
public:
    void sendToServer(Packet* packet);
};
