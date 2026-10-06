#pragma once

class Block;
class BlockSource {
public:
    Block* getBlock(int x, int y, int z);
    Block* getBlock(const glm::ivec3& pos);
};
