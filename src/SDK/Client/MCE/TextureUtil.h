#pragma once

#include "TextureFormat.h"

namespace mce {
    class TextureUtil {
    public:
        static int getBytesPerPixel(const TextureFormat format) {
            switch (format) {
                default:
                    return 0;
                case TextureFormat::R32g32b32a32Float:
                    return 16;
                case TextureFormat::R16g16b16a16Float:
                case TextureFormat::R16g16b16a16Unorm:
                case TextureFormat::R32g32Float:
                    return 8;
                case TextureFormat::R10g10b10a2Unorm:
                case TextureFormat::R11g11b10Float:
                case TextureFormat::R8g8b8a8Unorm:
                case TextureFormat::R8g8b8a8UnormSrgb:
                case TextureFormat::R16g16Float:
                case TextureFormat::R16g16Unorm:
                case TextureFormat::R16g16Uint:
                case TextureFormat::R16g16Snorm:
                case TextureFormat::D32Float:
                case TextureFormat::R32Float:
                case TextureFormat::R32Uint:
                case TextureFormat::R24g8Typeless:
                case TextureFormat::D24UnormS8Uint:
                case TextureFormat::R24UnormX8Typeless:
                case TextureFormat::Bc3Unorm:
                case TextureFormat::B8g8r8a8Unorm:
                case TextureFormat::B8g8r8a8UnormSrgb:
                case TextureFormat::Bc7Unorm:
                    return 4;
                case TextureFormat::R8g8Unorm:
                case TextureFormat::R8g8Snorm:
                case TextureFormat::R16Float:
                case TextureFormat::D16Unorm:
                    return 2;
                case TextureFormat::R8Unorm:
                case TextureFormat::R8Uint:
                case TextureFormat::A8Unorm:
                    return 1;
                case TextureFormat::R8g8b8Unorm:
                    return 3;
            }
        }
    };
}
