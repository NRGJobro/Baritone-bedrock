#include "LightTexture.h"

mce::Color LightTexture::getColorForUV(const BrightnessPair brightness) const {
    return this->getColorForUV(brightnessToUV(brightness));
}

mce::Color LightTexture::getColorForUV(const glm::vec2& uv) const {
    if (this->brightnessImage == nullptr || this->brightnessImage->isEmpty())
        return {0.f, 0.f, 0.f, 0.f};

    const auto data = this->brightnessImage->imageData.getData();

    if (data == nullptr)
        return {0.f, 0.f, 0.f, 0.f};

    float x = uv.x * 16.f - 0.5f;
    float y = uv.y * 16.f - 0.5f;

    if (x >= 0.f) {
        if (x >= 15.f)
            x = 14.f;
    }
    else
        x = 0.f;

    if (y >= 0.f) {
        if (y >= 15.f)
            y = 14.f;
    }
    else
        y = 0.f;

    const int xOff = static_cast<int>(x) - (x >= static_cast<int>(x) ? 0 : 1);
    const int yOff = static_cast<int>(y) - (y >= static_cast<int>(y) ? 0 : 1);

    const float diffX = x - static_cast<float>(xOff);
    const float oneMinDiffX = 1.f - diffX;
    const float diffY = y - static_cast<float>(yOff);
    const float oneMinDiffY = 1.f - diffY;

    mce::Color col1, col2, col3, col4;

    if (!this->isDeferred) {
        const int index = xOff + yOff * 16;
        const int col1Index = index * 4;
        const int col2Index = index * 4 + 4;
        const int col3Index = index * 4 + 64;
        const int col4Index = index * 4 + 68;

        const int i1 = *reinterpret_cast<int*>(&data[col1Index]);
        const int i2 = *reinterpret_cast<int*>(&data[col2Index]);
        const int i3 = *reinterpret_cast<int*>(&data[col3Index]);
        const int i4 = *reinterpret_cast<int*>(&data[col4Index]);

        col1 = mce::Color{i1 & 0xFF, (i1 >> 8) & 0xFF, (i1 >> 16) & 0xFF, (i1 >> 24) & 0xFF};
        col2 = mce::Color{i2 & 0xFF, (i2 >> 8) & 0xFF, (i2 >> 16) & 0xFF, (i2 >> 24) & 0xFF};
        col3 = mce::Color{i3 & 0xFF, (i3 >> 8) & 0xFF, (i3 >> 16) & 0xFF, (i3 >> 24) & 0xFF};
        col4 = mce::Color{i4 & 0xFF, (i4 >> 8) & 0xFF, (i4 >> 16) & 0xFF, (i4 >> 24) & 0xFF};
    }
    else {
        const int col1Index = yOff * 4;
        const int col2Index = yOff * 4 + 4;
        const int col3Index = xOff * 4 + 64;
        const int col4Index = xOff * 4 + 68;

        const int i1 = *reinterpret_cast<int*>(&data[col1Index]);
        const int i2 = *reinterpret_cast<int*>(&data[col2Index]);
        const int i3 = *reinterpret_cast<int*>(&data[col3Index]);
        const int i4 = *reinterpret_cast<int*>(&data[col4Index]);

        const auto tmpcol1 = mce::Color{i1 & 0xFF, (i1 >> 8) & 0xFF, (i1 >> 16) & 0xFF, (i1 >> 24) & 0xFF};
        const auto tmpcol2 = mce::Color{i2 & 0xFF, (i2 >> 8) & 0xFF, (i2 >> 16) & 0xFF, (i2 >> 24) & 0xFF};
        const auto tmpcol3 = mce::Color{i3 & 0xFF, (i3 >> 8) & 0xFF, (i3 >> 16) & 0xFF, (i3 >> 24) & 0xFF};
        const auto tmpcol4 = mce::Color{i4 & 0xFF, (i4 >> 8) & 0xFF, (i4 >> 16) & 0xFF, (i4 >> 24) & 0xFF};

        col1 = tmpcol3 + tmpcol1;
        col2 = tmpcol4 + tmpcol1;
        col3 = tmpcol2 + tmpcol3;
        col4 = tmpcol2 + tmpcol4;

        col1.r = std::clamp(col1.r, 0.f, 1.f);
        col1.g = std::clamp(col1.g, 0.f, 1.f);
        col1.b = std::clamp(col1.b, 0.f, 1.f);
        col1.a = std::clamp(col1.a, 0.f, 1.f);

        col2.r = std::clamp(col2.r, 0.f, 1.f);
        col2.g = std::clamp(col2.g, 0.f, 1.f);
        col2.b = std::clamp(col2.b, 0.f, 1.f);
        col2.a = std::clamp(col2.a, 0.f, 1.f);

        col3.r = std::clamp(col3.r, 0.f, 1.f);
        col3.g = std::clamp(col3.g, 0.f, 1.f);
        col3.b = std::clamp(col3.b, 0.f, 1.f);
        col3.a = std::clamp(col3.a, 0.f, 1.f);

        col4.r = std::clamp(col4.r, 0.f, 1.f);
        col4.g = std::clamp(col4.g, 0.f, 1.f);
        col4.b = std::clamp(col4.b, 0.f, 1.f);
        col4.a = std::clamp(col4.a, 0.f, 1.f);
    }

    return {
        (col2.r * diffX + col1.r * oneMinDiffX) * oneMinDiffY + (col3.r * diffX + col4.r * oneMinDiffX) * diffY,
        (col2.g * diffX + col1.g * oneMinDiffX) * oneMinDiffY + (col3.g * diffX + col4.g * oneMinDiffX) * diffY,
        (col2.b * diffX + col1.b * oneMinDiffX) * oneMinDiffY + (col3.b * diffX + col4.b * oneMinDiffX) * diffY,
        (col2.a * diffX + col1.a * oneMinDiffX) * oneMinDiffY + (col3.a * diffX + col4.a * oneMinDiffX) * diffY
    };
}

glm::vec2 LightTexture::brightnessToUV(const BrightnessPair brightness) {
    return {brightness.block.value / 256.f, brightness.sky.value / 256.f};
}