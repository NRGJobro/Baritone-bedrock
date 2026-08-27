#pragma once

template <typename T>
class OwnerStorageSharePtr {
public:
    enum class VariadicInit : int {
        NonAmbiguous
    };

    enum class EmptyInit : int {
        NoValue
    };

    std::shared_ptr<T> handle;

    T* get() const {
        return this->handle.get();
    }
};
