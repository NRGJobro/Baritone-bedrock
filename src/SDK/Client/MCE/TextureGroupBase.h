#pragma once

#include "TexturePtr.h"

namespace mce {
    class TextureGroupBase {
    public:
        virtual void Destructor();
        virtual TexturePtr getTexture(const ResourceLocation& location, bool forceReload = false, std::optional<int> opt = {},
            cg::TextureSetLayerType type = cg::TextureSetLayerType::Color);
    };
}
