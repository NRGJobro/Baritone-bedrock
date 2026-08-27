#include "Image.h"

#include "../../../Memory/Sig/SignatureManager.h"

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
    static auto sig = GET_SIG("mce::Image::clone");
    static auto func = *(decltype(&Image::clone)*)&sig;
    return (this->*func)();
}

bool mce::Image::isEmpty() const {
    return this->imageData.size == 0;
}
