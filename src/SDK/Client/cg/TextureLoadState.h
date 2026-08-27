#pragma once

enum class TextureLoadState : int8_t {
    UnloadedBit = 0x0,
    PendingBit = 0x1,
    LoadedBit = 0x2,
    PendingMetadata = 0x4,
    LoadedMetadata = 0x8
};
