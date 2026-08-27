#pragma once

namespace cg {
    enum class TextureSetLayerType : uint8_t {
        Color,
        ColorUnlit,
        Mer,
        Mers,
        Metalness,
        Emissive,
        Roughness,
        Subsurface,
        Normal,
        Heightmap,
        Count
    };
}
