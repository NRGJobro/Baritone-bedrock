#pragma once

#include "../../../Client/MCE/Color.h"
#include "../../Block/Block.h"

struct MapSample {
    mce::Color color;
    Block* block;
    int16_t height;
};
