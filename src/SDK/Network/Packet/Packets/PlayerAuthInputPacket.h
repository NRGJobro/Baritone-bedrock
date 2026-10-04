#pragma once

#include "../Packet.h"
#include "../../../World/Level/HitResult/FacingID.h"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

enum class PlayerActionType : int {
    StartDestroyBlock = 0,
    AbortDestroyBlock = 1,
    StopDestroyBlock = 2,
    CrackBlock = 18
};

struct PlayerBlockActionData {
    PlayerActionType type;
    glm::ivec3 pos;
    int face;
};

struct PackedItemUseLegacyInventoryTransaction {};
struct ItemStackRequestData {};

class PlayerAuthInputPacket : public Packet {
public:
    static constexpr auto packetId = MinecraftPacketIds::PlayerAuthInputPacket;

    float pitch;
    float yaw;
    glm::vec3 pos;
    float bodyYaw;
    glm::vec3 posDelta;
    glm::vec2 vehicleRotation;
    glm::vec2 analogMoveVector;
    glm::vec2 move;
    glm::vec2 interactRotation;
    glm::vec3 cameraOrientation;
    glm::vec2 rawMoveVector;
    std::bitset<65> inputFlags;
    int inputMode;
    int playMode;
    int interactionModel;
    std::uint64_t clientTick;
    std::unique_ptr<PackedItemUseLegacyInventoryTransaction> itemUseTransaction;
    std::unique_ptr<ItemStackRequestData> itemStackRequest;
    std::vector<PlayerBlockActionData> blockActions;
    std::uint64_t clientPredictedVehicle;
};

static_assert(sizeof(PlayerBlockActionData) == 0x14);
static_assert(sizeof(PlayerAuthInputPacket) == 0xE0);
