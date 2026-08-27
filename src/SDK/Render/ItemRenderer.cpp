#include "ItemRenderer.h"

#include "../../Memory/Sig/SignatureManager.h"
#include "../../Utils/Utils.h"

void ItemRenderer::renderGuiItemNew(BaseActorRenderContext *renderContext, ItemStack *item, int frame, float x, float y, bool forceEnchantmentFoil, float opacity, float lightMultiplier, float scale, int pass) {
	static auto sig = Utils::getFromOffset<uintptr_t>(GET_SIG("ItemRenderer::renderGuiItemNew"), 1);
    static auto renderGuiItemNew = *(decltype(&ItemRenderer::renderGuiItemNew)*)&sig;
    return (this->*renderGuiItemNew)(renderContext, item, frame, x, y, forceEnchantmentFoil, opacity, lightMultiplier, scale, pass);
}

void ItemRenderer::iconBlit(BaseActorRenderContext* renderContext, const mce::TexturePtr& texture, float x, float y, float z, const TextureUVCoordinateSet& iconTextureCoord, float w, float h,
    float lightMultiplier, float alphaMultiplier, int colorMultiplier, int secondaryColorMultiplier, float xscale, float yscale, IconBlitGlint iconBlitGlint, bool useMultiColorTextureTinting) {
    static auto sig = Utils::getFromOffset<uintptr_t>(GET_SIG("ItemRenderer::iconBlit"), 1);
    static auto func = *(decltype(&ItemRenderer::iconBlit)*)&sig;
    (this->*func)(renderContext, texture, x, y, z, iconTextureCoord, w, h, lightMultiplier, alphaMultiplier, colorMultiplier, secondaryColorMultiplier, xscale, yscale, iconBlitGlint, useMultiColorTextureTinting);
}

ItemGraphics& ItemRenderer::getGraphics(const ItemStack& stack) {
    using func_t = ItemGraphics&(*)(ItemRenderer*, const ItemStack&);
    static auto func = reinterpret_cast<func_t>(GET_SIG("ItemRenderer::getGraphics"));
    return func(this, stack);
}
