#include "MinecraftPackets.h"

std::shared_ptr<Packet> MinecraftPackets::createPacket(const MinecraftPacketIds id) {
    using func_t = std::shared_ptr<Packet>(*)(MinecraftPacketIds);
    static auto func = reinterpret_cast<func_t>(GET_SIG("MinecraftPackets::createPacket"));
    return func(id);
}
