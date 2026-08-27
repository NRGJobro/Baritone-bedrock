#pragma once

#include "../../../Memory/Sig/SignatureManager.h"
#include "Packet.h"

class MinecraftPackets {
public:
    static std::shared_ptr<Packet> createPacket(MinecraftPacketIds id);

    template<typename T>
    static std::shared_ptr<T> createPacket() {
        using func_t = std::shared_ptr<T>(__fastcall*)(MinecraftPacketIds);
        static auto func = reinterpret_cast<func_t>(GET_SIG("MinecraftPackets::createPacket"));
        return func(T::packetId);
    }
};
