#include "Blob.h"

mce::Blob::Blob(const value_type* data, const size_t size) {
    this->blob = {new value_type[size], Deleter()};
    this->size = size;

    memcpy(this->blob.get(), data, size);
}

mce::Blob::Blob(const Blob& other) {
    *this = other;
}

mce::Blob& mce::Blob::operator=(const Blob& other) {
    if (this->blob.get() != other.blob.get()) {
        if (this->blob != nullptr)
            this->blob.reset();

        this->size = other.size;

        if (other.size > 0) {
            this->blob = {new value_type[other.size], Deleter()};

            memcpy(this->blob.get(), other.blob.get(), other.size);
        }
    }

    return *this;
}

void mce::Blob::defaultDeleter(pointer p) {
    delete[] p;
}
