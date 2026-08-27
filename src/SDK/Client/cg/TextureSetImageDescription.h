#pragma once

#include "../mce/Color.h"
#include "ColorChannel.h"
#include "ImageDescription.h"
#include "TextureSetLayerType.h"

namespace cg {
    struct TextureSetImageDescription {
        struct LayerInfoVar {
            TextureSetLayerType layerType;
            std::variant<ImageDescription, ColorChannel, mce::Color> data;
        };

        std::vector<LayerInfoVar> layerInfo;
    };
}
