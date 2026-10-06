#pragma once

#include "EnableNonOwnerReferences.h"

namespace Bedrock {
    template<typename T>
    class NonOwnerPointerRef {
    public:
        std::shared_ptr<EnableNonOwnerReferences::ControlBlock> mControlBlock;
        T* self;
    };

}
