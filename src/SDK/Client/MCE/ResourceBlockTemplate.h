#pragma once

#include "PerFrameHandleTracker.h"
#include "ResourceServiceTextureDescription.h"

namespace mce {
    template<typename type_t>
    struct ResourceBlockTemplate {
        std::shared_ptr<ResourceServiceTextureDescription> debugInfoBLock = nullptr;
        PerFrameHandleTracker trackingBlock{};
        std::unique_ptr<type_t> resource = nullptr;

        std::shared_ptr<ResourceServiceTextureDescription> getDebugBlock() const {
            return this->debugInfoBLock;
        }

        type_t* get() {
            return this->resource.get();
        }

        void reset() {
            this->debugInfoBLock.reset();
            this->trackingBlock.resetValidity();
            this->resource.reset();
        }

        void resetValidity() {
            this->trackingBlock.resetValidity();
        }

        void setAsValid() {
            this->trackingBlock.setAsValid();
        }

        bool isValid(ValidityCheckType type) {
            return this->trackingBlock.isValid(type);
        }

        void invalidate() {
            this->trackingBlock.invalidate();
        }

        bool hasCheckedForValidity() const {
            return this->trackingBlock.hasCheckedForValidity();
        }
    };
}
