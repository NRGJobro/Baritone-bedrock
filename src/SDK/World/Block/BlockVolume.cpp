#include "BlockVolume.h"

BlockVolume::BlockVolume(const glm::ivec3& volume, const int16_t dimensionBottom) {
    this->width = volume.x;
    this->height = volume.y;
    this->depth = volume.z;
    this->dimensionBottom = dimensionBottom;
    this->blocks = buffer_span_mut<Block*>(this->width * this->height * this->depth);
}
