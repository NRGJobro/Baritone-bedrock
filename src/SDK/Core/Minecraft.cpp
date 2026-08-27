#include "Minecraft.h"

GameSession* Minecraft::getGameSession() {
    return hat::member_at<GameSession*>(this, 0xB8);
}

std::shared_ptr<EntityRegistry> Minecraft::getEntityRegistry() {
    return hat::member_at<std::shared_ptr<EntityRegistry>>(this, 0x100);
}
