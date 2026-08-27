#pragma once

template <typename T>
class SharedCounter {
public:
    constexpr explicit SharedCounter(T* p = nullptr) : ptr(p), shareCount(1), weakCount(0) {}

    constexpr void addShareCount() {
        this->shareCount++;
    }

    constexpr void addWeakCount() {
        this->weakCount++;
    }

    [[nodiscard]] constexpr int getShareCount() const {
        return this->shareCount.load();
    }

    [[nodiscard]] constexpr int getWeakCount() const {
        return this->weakCount.load();
    }

    constexpr T* get() const {
        return this->ptr;
    }

    constexpr void release() {
        if (--this->shareCount == 0) {
            delete this->ptr;
            this->ptr = nullptr;
            
            if (this->weakCount == 0)
                delete this;
        }
    }

    constexpr void releaseWeak() {
        if (--this->weakCount == 0 && this->shareCount == 0)
            delete this;
    }

private:
    T* ptr;
    std::atomic_int shareCount;
    std::atomic_int weakCount;
};
