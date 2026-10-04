#pragma once

#include "../../Util/HashedString.h"

namespace mce {
    class MaterialPtr {
        std::byte reserved[0x138];

    public:
        static MaterialPtr* createMaterial(const HashedString& name);
    };

    static_assert(sizeof(MaterialPtr) == 0x138);
}
