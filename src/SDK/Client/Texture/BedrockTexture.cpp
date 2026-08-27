#include "BedrockTexture.h"

void BedrockTexture::unload() {
    if (this->texture != nullptr) {
        this->texture->unload();
        this->texture = nullptr;
    }

    if (this->mersTexture != nullptr) {
        this->mersTexture->unload();
        this->mersTexture = nullptr;
    }

    if (this->normalTexture != nullptr) {
        this->normalTexture->unload();
        this->normalTexture = nullptr;
    }
}
