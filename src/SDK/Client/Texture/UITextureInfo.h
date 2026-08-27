#pragma once

#include "../ui/SliceSize.h"
#include "AsepriteFrameInformation.h"

struct UITextureInfo {
    ui::SliceSize sliceSize;
    bool hasNineslice;
    glm::vec2 baseUVSize;
    std::vector<AsepriteFrameInformation> asepriteFrames;
    int totalDurationInMilliseconds;
};
