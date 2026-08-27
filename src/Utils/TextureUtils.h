#pragma once

#include "../SDK/Client/MCE/ClientTexture.h"

class TextureUtils {
public:
    static mce::ClientTexture& uploadImageData(const std::vector<mce::Color>& colors, uint32_t width, uint32_t height, const std::string& uploadName);
    static mce::ClientTexture& uploadImageData(uint8_t* colors, uint32_t width, uint32_t height, const std::string& uploadName);
};
