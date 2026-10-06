#include "Player.h"
#include "LocalPlayer.h"

#include "../Inventory/PlayerInventory.h"
#include "GameMode.h"

PlayerInventory* Player::getSupplies() {
    return hat::member_at<PlayerInventory*>(this, 0x5B8);
}

GameMode* LocalPlayer::getGameMode() {
    return hat::member_at<GameMode*>(this, 0xAA0);
}
