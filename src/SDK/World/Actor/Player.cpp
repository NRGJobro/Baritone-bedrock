#include "Player.h"
#include "LocalPlayer.h"

PlayerInventory* Player::getSupplies() {
    return hat::member_at<PlayerInventory*>(this, 0x5B8);
}

GameMode* LocalPlayer::getGameMode() {
    auto& holder = hat::member_at<std::shared_ptr<GameMode>>(this, 0xAA0);
    return holder.get();
}
