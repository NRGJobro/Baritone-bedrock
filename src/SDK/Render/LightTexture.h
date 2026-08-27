#pragma once

#include "../Client/MCE/ClientTexture.h"
#include "../Client/MCE/DynamicTexture.h"
#include "../Client/MCE/Image.h"
#include "../World/Level/BrightnessPair.h"
#include "BaseLightData.h"

struct LightTexture {
    std::shared_ptr<mce::Image> brightnessImage;
    mce::DynamicTexture brightnessTextureId;
    std::shared_ptr<mce::ClientTexture> brightnessTexture;
    bool isTextureDirty;
    bool isDeferred;
    bool isSplitImage;
    BaseLightData lightData;
    std::shared_ptr<mce::Image> builtLightImage;

    [[nodiscard]] mce::Color getColorForUV(BrightnessPair brightness) const;
    [[nodiscard]] mce::Color getColorForUV(const glm::vec2& uv) const;

    static glm::vec2 brightnessToUV(BrightnessPair brightness);
};
