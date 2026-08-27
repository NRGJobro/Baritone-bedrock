#include "TextureContainer.h"

mce::TextureContainer::TextureContainer(Image& image) {
    this->storage = std::make_shared<cg::ImageResource>(image);
    this->description = TextureDescription(image);
    this->valid = true;
}

mce::TextureContainer::TextureContainer(cg::ImageBuffer& image) {
    this->storage = std::make_shared<cg::ImageResource>(image);
    this->description = TextureDescription(image.imageDescription);
    this->valid = true;
}
