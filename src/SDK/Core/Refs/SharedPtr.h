#pragma once

#include "SharedCounter.h"

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr {
public:
    template <typename... Args>
    [[nodiscard]] static SharedPtr make(Args&&... args) {
        return SharedPtr(new T(std::forward<Args>(args)...));
    }

    [[nodiscard]] SharedPtr() noexcept : counter(nullptr) {}
    [[nodiscard]] SharedPtr(std::nullptr_t) noexcept : counter(nullptr) {}

    [[nodiscard]] explicit SharedPtr(T* p) : counter(new SharedCounter<T>(p)) {}

    template <class Y>
    [[nodiscard]] explicit SharedPtr(const SharedPtr<Y>& other) requires(std::convertible_to<Y*, T*>) {
        this->counter = static_cast<SharedCounter<T>*>(other.counter);

        if (this->counter)
            this->counter->addShareCount();
    }

    template <class Y>
    [[nodiscard]] explicit SharedPtr(SharedPtr<Y>&& other) requires(std::convertible_to<Y*, T*>) {
        this->counter = static_cast<SharedCounter<T>*>(other.counter);
        other.counter = nullptr;
    }

    template <class Y>
    [[nodiscard]] explicit SharedPtr(const WeakPtr<Y>& other) requires(std::convertible_to<Y*, T*>) {
        this->counter = static_cast<SharedCounter<T>*>(other.counter);

        if (other)
            this->counter->addShareCount();
    }

    ~SharedPtr() {
        if (this->counter)
            this->counter->release();
    }

    template <class Y>
    SharedPtr& operator=(const SharedPtr<Y>& other) requires(std::convertible_to<Y*, T*>) {
        if (this->counter != static_cast<SharedCounter<T>*>(other.counter)) {
            this->counter = static_cast<SharedCounter<T>*>(other.counter);

            if (this->counter)
                this->counter->addShareCount();
        }

        return *this;
    }

    template <class Y>
    SharedPtr& operator=(SharedPtr<Y>&& other) requires(std::convertible_to<Y*, T*>) {
        if (this->counter != static_cast<SharedCounter<T>*>(other.counter)) {
            this->counter = static_cast<SharedCounter<T>*>(other.counter);
            other.counter = nullptr;
        }

        return *this;
    }

    template <class Y>
    SharedPtr& operator=(const WeakPtr<Y>& other) requires(std::convertible_to<Y*, T*>) {
        this->counter = static_cast<SharedCounter<T>*>(other.counter);

        if (other)
            this->counter->addShareCount();

        return *this;
    }

    [[nodiscard]] T* get() const {
        return this->counter ? this->counter->get() : nullptr;
    }

    [[nodiscard]] T* operator->() const {
        return this->get();
    }

    [[nodiscard]] T& operator*() const {
        return *this->get();
    }

    [[nodiscard]] explicit operator bool() const {
        return this->get() != nullptr;
    }

    [[nodiscard]] int use_count() const {
        return this->counter ? this->counter->getShareCount() : 0;
    }

    void reset() {
        if (this->counter) {
            this->counter->release();
            this->counter = nullptr;
        }
    }

    SharedCounter<T>* counter;
};
