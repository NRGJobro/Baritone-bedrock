#pragma once

#include "TextureWrapping.h"

namespace mce {
    struct TextureWrappingDescription {
        TextureWrapping uAddress;
        TextureWrapping vAddress;
        TextureWrapping wAddress;
    };
}
