#pragma once

template <typename T>
class StackResultStorageSharePtr {
public:
    std::shared_ptr<T> handle;

    T* get() const {
        return this->handle.get();
    }
};
