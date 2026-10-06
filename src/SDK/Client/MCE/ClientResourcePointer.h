#pragma once

namespace mce {
    // Limiter only needs the native ResourcePointer ABI shell carried inside
    // Mesh. The actual resource type is never dereferenced, so keep the
    // vtable/shared_ptr footprint and reference-count helper without importing
    // the large buffer/resource SDK hierarchy.
    template<typename type_t = void>
    struct ClientResourcePointer {
        void* vtable = nullptr;
        std::shared_ptr<void> resourcePointerBlock{};

        void incRef() const {
            *reinterpret_cast<int*>(*reinterpret_cast<uintptr_t*>(
                reinterpret_cast<uintptr_t>(this) + 0x10) + 0x8) += 1;
        }

        void decRef() const {
            *reinterpret_cast<int*>(*reinterpret_cast<uintptr_t*>(
                reinterpret_cast<uintptr_t>(this) + 0x10) + 0x8) -= 1;
        }

        void setRef(const int count) const {
            *reinterpret_cast<int*>(*reinterpret_cast<uintptr_t*>(
                reinterpret_cast<uintptr_t>(this) + 0x10) + 0x8) = count;
        }

        int getRefCount() const {
            return *reinterpret_cast<int*>(*reinterpret_cast<uintptr_t*>(
                reinterpret_cast<uintptr_t>(this) + 0x10) + 0x8);
        }
    };

    static_assert(sizeof(ClientResourcePointer<void>) == 0x18);
}
