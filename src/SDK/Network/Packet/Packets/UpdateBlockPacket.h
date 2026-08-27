#pragma once

#include "../Packet.h"

class UpdateBlockPacket : public Packet {
public:
    static constexpr auto packetId = MinecraftPacketIds::UpdateBlock;

    enum class BlockLayer : uint32_t {
        Standard,
        Extra,
        Count
    };

    enum class BlockUpdateFlag : uint8_t {
        None = 0,
        Neighbors = 1 << 0,
        Network = 1 << 1,
        NoGraphic = 1 << 2,
        Priority = 1 << 3,
        ForceNoticeListener = 1 << 4,
        All = Neighbors | Network,
        AllPriority = All | Priority
    };

    glm::ivec3 pos;
    BlockLayer layer;
    BlockUpdateFlag updateFlags;
    uint32_t runtimeId;
};
