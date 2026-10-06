#include "Packet.h"

#include "../../../Utils/Utils.h"

MinecraftPacketIds Packet::getID() {
    return Utils::CallVFunc<1, MinecraftPacketIds>(this);
}
