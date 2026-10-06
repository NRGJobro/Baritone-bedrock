#pragma once

template<typename T, size_t Size>
class StaticVector {
    T data[Size]{};
    size_t size{};

public:
    StaticVector() = default;

    void push_back(const T& element) {
        if (size == Size)
            return;

        data[size] = element;
        size++;
    }

    size_t getSize() const {
        return size;
    }

    T* operator[](const size_t index) {
        if (index >= size)
            return nullptr;

        return &data[index];
    }
};
