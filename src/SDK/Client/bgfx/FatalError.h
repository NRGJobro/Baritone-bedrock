#pragma once

namespace bgfx::Fatal {
    enum Enum : int {
        DebugCheck,
        InvalidShader,
        UnableToInitialize,
        UnableToCreateTexture,
        DeviceLost,
        Count
    };
}

namespace bgfx {
    struct FatalError {
        Fatal::Enum error;
        int hResult;
    };
}
