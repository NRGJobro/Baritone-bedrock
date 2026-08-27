#include "ImageResource.h"

#include "../../../Memory/Sig/SignatureManager.h"
#include "../../../Utils/Utils.h"

cg::ImageResource::ImageResource() {
    static auto sig = Utils::getFromOffset<void*>(GET_SIG("cg::ImageResourceVtable"), 3);
    *reinterpret_cast<void**>(this) = sig;
}

cg::ImageResource::ImageResource(mce::Image& image) {
    static auto sig = Utils::getFromOffset<void*>(GET_SIG("cg::ImageResourceVtable"), 3);
    *reinterpret_cast<void**>(this) = sig;

    this->storage.emplace_back(image);
}

cg::ImageResource::ImageResource(ImageBuffer& image) {
    static auto sig = Utils::getFromOffset<void*>(GET_SIG("cg::ImageResourceVtable"), 3);
    *reinterpret_cast<void**>(this) = sig;

    this->storage.emplace_back(image);
}

/*bool cg::ImageResource::isEmpty() {
    return this->storage.empty();
}

bool cg::ImageResource::isValid() {
    for (const auto& image : this->storage) {
        if (!image.isValid())
            return false;
    }

    return true;
}

size_t cg::ImageResource::getSize() {
    return this->storage.size();
}

cg::ImageBuffer& cg::ImageResource::getImage(const uint32_t index) {
    return this->storage.at(index);
}

void cg::ImageResource::addImage(const ImageBuffer& image) {
    this->storage.emplace_back(image);
}

std::variant<std::vector<cg::ImageBuffer>, std::pair<std::vector<cg::ImageBuffer>, uint64_t>> cg::ImageResource::unwrapImageData() {
    return this->storage;
}*/
