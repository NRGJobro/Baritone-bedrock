#pragma once

#include "ResourceBlockTemplate.h"
#include "ValidityCheckType.h"

namespace mce {
    template<typename type_t, typename block_t = ResourceBlockTemplate<type_t>, template<typename> typename pointer_t = std::shared_ptr>
    struct ResourcePointer {
        void* vtable;
        pointer_t<block_t> resourcePointerBlock;

        bool isValid(ValidityCheckType checkType = ValidityCheckType::Increment) {
            if (this->resourcePointerBlock == nullptr)
                return false;

            return this->resourcePointerBlock->isValid(checkType);
        }

        void incRef() const {
            *reinterpret_cast<int*>(*reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(this) + 0x10) + 0x8) += 1;
        }

        void decRef() const {
            *reinterpret_cast<int*>(*reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(this) + 0x10) + 0x8) -= 1;
        }

        void setRef(const int count) const {
            *reinterpret_cast<int*>(*reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(this) + 0x10) + 0x8) = count;
        }

        int getRefCount() const {
            return *reinterpret_cast<int*>(*reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(this) + 0x10) + 0x8);
        }
    };
}
