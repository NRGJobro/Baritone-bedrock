#include "MeshContext.h"

#include "../../MC.h"

void mce::MeshContext::setClippingRectangle(const float x, const float y, const float width, const float height) {
    const auto& clientUIScreenSize = MC::getGuiData()->screenSizeData.clientUIScreenSize;

    const float normWidth = 1.f / clientUIScreenSize.x;
    const float normHeight = 1.f / clientUIScreenSize.y;

    const float minX = std::clamp(x * normWidth, 0.f, 1.f);
    const float maxX = std::clamp(width * normWidth, 0.f, 1.f);
    const float minY = std::clamp(y * normHeight, 0.f, 1.f);
    const float maxY = std::clamp(height * normHeight, 0.f, 1.f);

    this->normalizedClipRegion = {minX, minY, maxX, maxY};
}

void mce::MeshContext::resetClippingRectangle() {
    this->normalizedClipRegion.reset();
}
