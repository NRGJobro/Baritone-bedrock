#include "Level.h"

HitResult* Level::getHitResult() {
    return hat::member_at<HitResult*>(this, 0x1E8);
}
