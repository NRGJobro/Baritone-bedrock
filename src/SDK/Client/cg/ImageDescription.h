#pragma once

#include "../cg/ImageType.h"
#include "../mce/Image.h"
#include "../mce/TextureFormat.h"
#include "../mce/TextureUtil.h"
#include "ColorSpace.h"

namespace cg {
    struct ImageDescription {
        uint32_t width = 0;
        uint32_t height = 0;
        mce::TextureFormat textureFormat = mce::TextureFormat::UnknownTextureFormat;
        ColorSpace colorSpace = ColorSpace::Unknown;
        ImageType imageType = ImageType::Texture2D;
        uint32_t arraySize = 1;

        ImageDescription() = default;
        ImageDescription(uint32_t width, uint32_t height, mce::TextureFormat format, ColorSpace colorSpace, ImageType imageType = ImageType::Texture2D, uint32_t arraySize = 1)
            : width(width), height(height), textureFormat(format), colorSpace(colorSpace), imageType(imageType), arraySize(arraySize) { }
        explicit ImageDescription(const mce::Image& image) : width(image.width), height(image.height) {
            switch(image.imageFormat) {
                case mce::ImageFormat::R8Unorm:
                    this->textureFormat = mce::TextureFormat::R8Unorm;
                    break;
                case mce::ImageFormat::RG8Unorm:
                    this->textureFormat = mce::TextureFormat::R8g8Unorm;
                    break;
                case mce::ImageFormat::RGB8Unorm:
                    this->textureFormat = mce::TextureFormat::R8g8b8Unorm;
                    break;
                case mce::ImageFormat::RGBA8Unorm:
                    this->textureFormat = mce::TextureFormat::R8g8b8a8Unorm;
                    break;
                case mce::ImageFormat::RGBA16Float:
                    this->textureFormat = mce::TextureFormat::R16g16b16a16Float;
                    break;
                case mce::ImageFormat::UnknownFormat:
                default:
                    this->textureFormat = mce::TextureFormat::UnknownTextureFormat;
                    break;
            }

            switch (image.usage) {
                case mce::ImageUsage::Data:
                    this->colorSpace = ColorSpace::Linear;
                    break;
                case mce::ImageUsage::sRGB:
                    this->colorSpace = ColorSpace::sRGB;
                    break;
                case mce::ImageUsage::UnknownUsage:
                default:
                    this->colorSpace = ColorSpace::Unknown;
                    break;
            }
        }

        explicit ImageDescription(const ImageDescription& desc) {
            this->width = desc.width;
            this->height = desc.height;
            this->textureFormat = desc.textureFormat;
            this->colorSpace = desc.colorSpace;
            this->imageType = desc.imageType;
            this->arraySize = desc.arraySize;
        }

        int getRequiredBufferSize() const {
            const int iFormat = *reinterpret_cast<const int*>(&this->textureFormat);

            if (iFormat < 117 || iFormat == 145) {
                const int a = this->imageType == ImageType::CubemapDeprecated ? 6 : this->arraySize, b = this->imageType == ImageType::TextureCube ? 6 : 1;

                int c = 0;

                if (this->width != 0 && this->height != 0)
                    c = mce::TextureUtil::getBytesPerPixel(this->textureFormat) * this->width * this->height;

                return a * c * b;
            }

            const auto width = std::max(this->width, 1u);
            const auto height = std::max(this->height, 1u);

            int bytesWidth, bytesHeight;

            switch (this->textureFormat) {
                default: {
                    bytesWidth = bytesHeight = 4;
                    break;
                }
                case mce::TextureFormat::Astc5x4:
                case mce::TextureFormat::Astc5x4Srgb: {
                    bytesWidth = 5;
                    bytesHeight = 4;
                    break;
                }
                case mce::TextureFormat::Astc5x5:
                case mce::TextureFormat::Astc5x5Srgb: {
                    bytesWidth = 5;
                    bytesHeight = 5;
                    break;
                }
                case mce::TextureFormat::Astc6x5:
                case mce::TextureFormat::Astc6x5Srgb: {
                    bytesWidth = 6;
                    bytesHeight = 5;
                    break;
                }
                case mce::TextureFormat::Astc6x6:
                case mce::TextureFormat::Astc6x6Srgb: {
                    bytesWidth = 6;
                    bytesHeight = 6;
                    break;
                }
                case mce::TextureFormat::Astc8x5:
                case mce::TextureFormat::Astc8x5Srgb: {
                    bytesWidth = 8;
                    bytesHeight = 5;
                    break;
                }
                case mce::TextureFormat::Astc8x6:
                case mce::TextureFormat::Astc8x6Srgb: {
                    bytesWidth = 8;
                    bytesHeight = 6;
                    break;
                }
                case mce::TextureFormat::Astc8x8:
                case mce::TextureFormat::Astc8x8Srgb: {
                    bytesWidth = 8;
                    bytesHeight = 8;
                    break;
                }
                case mce::TextureFormat::Astc10x5:
                case mce::TextureFormat::Astc10x5Srgb: {
                    bytesWidth = 10;
                    bytesHeight = 5;
                    break;
                }
                case mce::TextureFormat::Astc10x6:
                case mce::TextureFormat::Astc10x6Srgb: {
                    bytesWidth = 10;
                    bytesHeight = 6;
                    break;
                }
                case mce::TextureFormat::Astc10x8:
                case mce::TextureFormat::Astc10x8Srgb: {
                    bytesWidth = 10;
                    bytesHeight = 8;
                    break;
                }
                case mce::TextureFormat::Astc10x10:
                case mce::TextureFormat::Astc10x10Srgb: {
                    bytesWidth = 10;
                    bytesHeight = 10;
                    break;
                }
                case mce::TextureFormat::Astc12x10:
                case mce::TextureFormat::Astc12x10Srgb: {
                    bytesWidth = 12;
                    bytesHeight = 10;
                    break;
                }
                case mce::TextureFormat::Astc12x12:
                case mce::TextureFormat::Astc12x12Srgb: {
                    bytesWidth = 12;
                    bytesHeight = 12;
                    break;
                }
            }

            return (width - 1 + bytesWidth) / bytesWidth * ((height - 1 + bytesHeight) / bytesHeight) * 16;
        }
    };
}
