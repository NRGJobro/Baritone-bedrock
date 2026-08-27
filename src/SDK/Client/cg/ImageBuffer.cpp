#include "ImageBuffer.h"

cg::ImageBuffer::ImageBuffer(const mce::Image& image) {
    this->storage = image.imageData;
    this->imageDescription = ImageDescription(image);
}

bool cg::ImageBuffer::isValid() const {
    return this->storage.size == this->imageDescription.getRequiredBufferSize();
}
