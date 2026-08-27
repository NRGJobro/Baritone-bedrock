#pragma once

#include "ImageInfo.h"

struct NinesliceInfo {
    ImageInfo topLeft;
    ImageInfo topRight;
    ImageInfo bottomLeft;
    ImageInfo bottomRight;
    glm::vec2 uvScale;
    std::vector<ImageInfo> left;
    std::vector<ImageInfo> top;
    std::vector<ImageInfo> right;
    std::vector<ImageInfo> bottom;
    std::vector<ImageInfo> middle;
};
