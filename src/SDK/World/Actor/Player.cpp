#include "Player.h"

PlayerInventory* Player::getSupplies() {
    return hat::member_at<PlayerInventory*>(this, 0x5B8);
}
