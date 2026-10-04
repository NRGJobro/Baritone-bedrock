#include "Block.h"

BlockLegacy* Block::getBlockLegacy() {
    return hat::member_at<BlockLegacy*>(this, 0x68);
}
