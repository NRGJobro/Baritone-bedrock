#include "LevelData.h"

int LevelData::getTime() {
    return hat::member_at<int>(this, 0x340);
}
