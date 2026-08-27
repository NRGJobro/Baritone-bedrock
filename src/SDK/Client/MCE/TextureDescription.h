#pragma once

#include "../cg/TextureDescription.h"
#include "BindFlagsBit.h"
#include "Color.h"
#include "SampleDescription.h"

namespace mce {
    struct TextureDescription : cg::TextureDescription {
        SampleDescription sampleDescription{};
        Color clearColor{1.f, 1.f, 1.f, 1.f};
        float optimizedClearDepth = 0.f;
        uint8_t optimizedClearStencil = 0;
        BindFlagsBit bindFlags = BindFlagsBit::ShaderResourceBit;
        bool isStaging = false;

        TextureDescription() = default;
        explicit TextureDescription(const Image& image) : cg::TextureDescription(image) { }
        explicit TextureDescription(const ImageDescription& desc) : cg::TextureDescription(desc) { }
    };
}
