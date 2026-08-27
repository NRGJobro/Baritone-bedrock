#pragma once

#include "ResourcePointer.h"

namespace mce {
    template<typename type_t, typename pointer_t = ResourcePointer<type_t>>
    struct ClientResourcePointer : pointer_t { };
}
