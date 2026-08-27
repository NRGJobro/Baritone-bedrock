#pragma once

#include "ValidityCheckType.h"

namespace mce {
    struct PerFrameHandleTracker {
        std::atomic<uint16_t> checkCount;
        std::atomic<bool> valid;

        void resetValidity() {
            this->checkCount = 0;
            this->valid = false;
        }

        bool isValid(const ValidityCheckType type) {
            if (type == ValidityCheckType::Increment)
                this->checkCount = this->checkCount + 1;

            return this->valid;
        }

        bool hasCheckedForValidity() const {
            return this->checkCount != 0;
        }

        void invalidate() {
            this->valid = false;
        }

        void setAsValid() {
            this->valid = true;
        }
    };
}
