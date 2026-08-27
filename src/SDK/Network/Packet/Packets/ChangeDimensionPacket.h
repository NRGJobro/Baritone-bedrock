#pragma once

#include "../../../World/DimensionID.h"
#include "../Packet.h"

class ChangeDimensionPacket : public Packet {
public:
    static constexpr auto packetId = MinecraftPacketIds::ChangeDimension;

    DimensionID dimensionId;
    glm::vec3 position;
    bool respawn;
    std::optional<uint32_t> loadingScreenId;
};
