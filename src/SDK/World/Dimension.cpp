#include "Dimension.h"

int16_t Dimension::getMinHeight() {
    return hat::member_at<int16_t>(this, 0xC8);
}

std::shared_ptr<BlockSource> Dimension::getBlockSource() {
    return hat::member_at<std::shared_ptr<BlockSource>>(this, 0xF0);
}
