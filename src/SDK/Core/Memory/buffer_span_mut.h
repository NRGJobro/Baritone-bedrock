#pragma once

template<typename T>
class buffer_span_mut {
public:
    T* begin = nullptr;
    T* end = nullptr;

    buffer_span_mut() = default;

    explicit buffer_span_mut(size_t size) {
        this->begin = new T[size];
        this->end = this->begin + size;

        memset(this->begin, 0, size * sizeof(T));
    }

    ~buffer_span_mut() {
        memset(this->begin, 0, reinterpret_cast<uintptr_t>(this->end) - reinterpret_cast<uintptr_t>(this->begin));
        delete[] this->begin;
        this->begin = nullptr;
        this->end = nullptr;
    }

    buffer_span_mut& operator=(const buffer_span_mut& other) {
        if (this != &other) {
            delete[] this->begin;
            this->begin = other.begin;
            this->end = other.end;
        }

        return *this;
    }

    [[nodiscard]] size_t getSize() const {
        return this->end - this->begin;
    }

    T& get(const uint32_t index) {
        return this->begin[index];
    }
};
