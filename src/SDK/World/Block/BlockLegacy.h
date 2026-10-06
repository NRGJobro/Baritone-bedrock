#pragma once

class Material;

class BlockSource;
class Block;

class BlockLegacy {
public:
    int16_t getBlockId();
    [[nodiscard]] const std::string& getName() const;
    bool isSolid();
    Material* getMaterial();
};
