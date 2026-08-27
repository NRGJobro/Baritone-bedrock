#pragma once

#include "../../Client/MCE/Color.h"
#include "../../Core/Refs/WeakPtr.h"
#include "Item.h"

class ItemStackBase {
    void** vtable;

public:
    WeakPtr<Item> item;

    [[nodiscard]] bool isValid() const;

    Item* getItem() const;

    [[nodiscard]] mce::Color getColor() const;
};
