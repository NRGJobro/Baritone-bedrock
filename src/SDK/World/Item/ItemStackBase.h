#pragma once

#include "../../Core/Refs/WeakPtr.h"
class Item;
class Block;
class BlockLegacy;
class CompoundTag;

class ItemStackBase {
public:
    void** vtable;
    WeakPtr<Item> item;
    CompoundTag* userData;
    const Block* block;
    std::int16_t auxValue;
    std::int8_t count;
    bool valid;
    bool showPickup;
    bool wasPickedUp;
    std::byte alignment[2];
    std::chrono::steady_clock::time_point pickupTime;
    std::byte pickupTimeReserved[8];
    std::vector<const BlockLegacy*> canPlaceOn;
    std::size_t canPlaceOnHash;
    std::vector<const BlockLegacy*> canDestroy;
    std::size_t canDestroyHash;
    std::uint64_t blockingTick;
    void* chargedItem;

    [[nodiscard]] bool isValid() const;

    Item* getItem() const;

    [[nodiscard]] bool isBlockType() const {
        return block != nullptr;
    }
};

static_assert(offsetof(ItemStackBase, item) == 0x8);
static_assert(offsetof(ItemStackBase, block) == 0x18);
static_assert(offsetof(ItemStackBase, valid) == 0x23);
static_assert(offsetof(ItemStackBase, canPlaceOn) == 0x38);
static_assert(sizeof(ItemStackBase) == 0x88);
