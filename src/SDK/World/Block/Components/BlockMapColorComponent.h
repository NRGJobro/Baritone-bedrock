#pragma once

#include "../TintMethod.h"
#include "../../../Client/MCE/Color.h"

struct BlockMapColorComponent {
    mce::Color color;
    TintMethod tintMethod;
};
