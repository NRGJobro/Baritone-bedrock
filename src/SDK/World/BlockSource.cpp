#include "BlockSource.h"

#include "Block/Block.h"

#include "../../Utils/Utils.h"

Block* BlockSource::getBlock(int x, int y, int z) {
    return this->getBlock({x, y, z});
}

Block* BlockSource::getBlock(const glm::ivec3& pos) {
    return Utils::CallVFunc<2, Block*, const glm::ivec3&>(this, pos);
}
