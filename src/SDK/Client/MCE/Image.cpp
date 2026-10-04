#include "Image.h"

mce::Image& mce::Image::operator=(const Image& other) {
    this->imageFormat = other.imageFormat;
    this->width = other.width;
    this->height = other.height;
    this->depth = other.depth;
    this->usage = other.usage;
    this->imageData = other.imageData;

    return *this;
}

mce::Image mce::Image::clone() const {
    Image result{};
    result = *this;
    return result;
}

bool mce::Image::isEmpty() const {
    return this->imageData.size == 0;
}
