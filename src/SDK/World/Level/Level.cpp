#include "Level.h"

HitResultWrapper* Level::getHitResultWrapper() {
    return hat::member_at<HitResultWrapper*>(this, 0x1E8);
}
