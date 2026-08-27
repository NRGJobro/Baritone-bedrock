#pragma once

template<typename T, size_t Size>
class StaticVector {
    T data[Size]{};
    size_t size{};

public:
    StaticVector() {
        memset(this, 0, sizeof(StaticVector));
    }

    void push_back(const T& element) {
        if (size == Size)
            return;

        data[size] = std::move(element);
        size++;
    }

    size_t getSize() const {
        return size;
    }

    T* operator[](const size_t index) {
        if (index >= Size)
            return nullptr;

        return &data[index];
    }
};
