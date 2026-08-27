#pragma once

#include "RenderMaterialInfo.h"

namespace mce {
    class MaterialPtr {
    public:
        std::shared_ptr<RenderMaterialInfo> materialInfo;

        static MaterialPtr* createMaterial(const HashedString& name);
    };
}
