#include "ItemIconManager.h"

#include "../../Memory/Sig/SignatureManager.h"

const TextureUVCoordinateSet& ItemIconManager::getIcon(const ItemStack& stack, const int newAnimationFrame, const bool isInventoryPane) {
    using func_t = const TextureUVCoordinateSet&(*)(const ItemStack&, int, bool);
    static auto func = reinterpret_cast<func_t>(GET_SIG("ItemIconManager::getIcon"));
    return func(stack, newAnimationFrame, isInventoryPane);
}
