#pragma once

namespace mce {
    enum BlendTarget : uint8_t {
        DestColor,
        SourceColor,
        Zero,
        One,
        OneMinusDestColor,
        OneMinusSrcColor,
        SourceAlpha,
        DestAlpha,
        OneMinusSrcAlpha8,
        UnknownBlendTarget
    };
}
