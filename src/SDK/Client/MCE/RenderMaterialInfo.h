#pragma once

#include "../../Util/HashedString.h"

namespace mce {
    class RenderMaterialInfo : std::enable_shared_from_this<RenderMaterialInfo> {
    public:
        HashedString name;

    private:
        char pad[0x8];
    };
}
