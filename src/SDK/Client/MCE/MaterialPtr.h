#pragma once

#include "../../Util/HashedString.h"

#include <memory>

namespace mce {
    class MaterialPtr {
        std::byte reserved[0x138];

    public:
        static std::shared_ptr<MaterialPtr> createMaterial(const HashedString& name);
    };

    static_assert(sizeof(MaterialPtr) == 0x138);
}
