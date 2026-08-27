#pragma once

#include "../cg/BufferDescription.h"

namespace dragon {
    struct BufferDescription : cg::BufferDescription {
        std::string debugName;
    };
}
