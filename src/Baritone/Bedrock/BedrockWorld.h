#pragma once

#include "../Core/World.h"

class BlockSource;

namespace baritone {

class BedrockWorld final : public IWorld {
    BlockSource* region;

public:
    explicit BedrockWorld(BlockSource* region);
    [[nodiscard]] BlockState getBlock(const BlockPos& pos) const override;
};

} // namespace baritone
