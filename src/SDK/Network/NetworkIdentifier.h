#pragma once

#include "RakNet/RakNetGUID.h"
#include "nether_net/NetworkID.h"

struct NetworkIdentifier {
    enum class Type : int {
        RakNet,
        Address,
        Address6,
        NetherNet,
        Invalid
    };
    
    NetherNet::NetworkID netherNetId;
    RakNet::RakNetGUID guid;

private:
    char pad[0x80]; // sock;

public:
    Type type;
};
