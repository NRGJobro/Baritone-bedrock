#pragma once

template <typename T>
class WeakStorageSharePtr {
public:
    enum class VariadicInit : int {
        NonAmbiguous
    };

    enum class EmptyInit : int {
        NoValue
    };

    std::weak_ptr<T> handle;

    T* get() const {
        return this->handle.lock().get();
    }
};
