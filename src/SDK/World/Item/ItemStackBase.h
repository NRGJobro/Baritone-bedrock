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

    [[nodiscard]] bool isBlockType() const {
        return hat::member_at<const Block*>(this, 0x18) != nullptr;
    }

    [[nodiscard]] mce::Color getColor() const;
};
