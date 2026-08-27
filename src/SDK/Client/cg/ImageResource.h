#pragma once

#include "ImageBuffer.h"

namespace cg {
    class ImageResource {
        void** vtable{};

    public:
        std::vector<ImageBuffer> storage{};

        ImageResource();
        explicit ImageResource(mce::Image& image);
        explicit ImageResource(ImageBuffer& image);

        /*virtual ~ImageResource() = default;
        virtual bool isEmpty();
        virtual bool isValid();
        virtual size_t getSize();
        virtual ImageBuffer& getImage(uint32_t index);
        virtual void addImage(std::shared_ptr<ImageResource> resource) {}
        virtual void addImage(const ImageBuffer& image);
        virtual std::variant<std::vector<ImageBuffer>, std::pair<std::vector<ImageBuffer>, uint64_t>> unwrapImageData();*/
    };
}
