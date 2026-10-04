#include "ItemStackBase.h"

bool ItemStackBase::isValid() const {
    return valid && count > 0 && item.get() != nullptr;
}

Item* ItemStackBase::getItem() const {
    return this->item.get();
}
