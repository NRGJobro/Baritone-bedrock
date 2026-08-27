#include "ItemStackBase.h"

#include "../../../Memory/Sig/SignatureManager.h"

bool ItemStackBase::isValid() const {
    return this->item.counter != nullptr;
}

Item* ItemStackBase::getItem() const {
    return this->item.get();
}

mce::Color ItemStackBase::getColor() const {
    using func_t = void(*)(const ItemStackBase*, mce::Color&);
    static auto func = reinterpret_cast<func_t>(GET_SIG("ItemStackBase::getColor"));
    mce::Color buf{};
    func(this, buf);
    return buf;
}
