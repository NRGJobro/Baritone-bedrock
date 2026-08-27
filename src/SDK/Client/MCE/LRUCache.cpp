#include "LRUCache.h"

#include "../../../Memory/Sig/SignatureManager.h"

void mce::LRUCache::remove(const ResourceLocation& location) {
    static auto sig = GET_SIG("mce::LRUCache::remove");
    static auto func = *(decltype(&LRUCache::remove)*)&sig;
    (this->*func)(location);
}
