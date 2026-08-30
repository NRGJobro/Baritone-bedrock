#pragma once

class BlockLegacy;
class Block;
class ItemStackBase;

class Item {
public:
    float getDestroySpeed(ItemStackBase* stack, Block* block);
    // Block items own a BlockLegacy pointer at this same-version offset.
    [[nodiscard]] bool isBlock() const {
        auto* legacy = *reinterpret_cast<BlockLegacy* const*>
            (reinterpret_cast<const std::byte*>(this) + 0x178);
        return legacy != nullptr;
    }
};
