#include "LoopbackPacketSender.h"

#include "../../Utils/Utils.h"

void LoopbackPacketSender::sendToServer(Packet* packet) {
    Utils::CallVFunc<4, void>(this, packet);
}
