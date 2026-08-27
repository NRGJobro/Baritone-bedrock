#pragma once

#include "BaseActorRenderContext.h"
#include "IconBlitGlint.h"
#include "ItemGraphics.h"
#include "TextureUVCoordinateSet.h"

class ItemRenderer {
public:
    void renderGuiItemNew(BaseActorRenderContext* renderContext, ItemStack* item, int frame, float x, float y, bool forceEnchantmentFoil, float opacity, float lightMultiplier, float scale, int pass = 17);
    void iconBlit(BaseActorRenderContext* renderContext, const mce::TexturePtr& texture, float x, float y, float z, const TextureUVCoordinateSet& iconTextureCoord, float w, float h, float lightMultiplier,
        float alphaMultiplier, int colorMultiplier, int secondaryColorMultiplier, float xscale, float yscale, IconBlitGlint iconBlitGlint, bool useMultiColorTextureTinting);
    ItemGraphics& getGraphics(const ItemStack& stack);
};
