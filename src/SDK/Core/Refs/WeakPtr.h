#pragma once

#include "SharedCounter.h"

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr {
public:
    [[nodiscard]] WeakPtr() noexcept : counter(nullptr) {}
    [[nodiscard]] WeakPtr(std::nullptr_t) noexcept : counter(nullptr) {}

    template <class Y>
    [[nodiscard]] explicit WeakPtr(const SharedPtr<Y>& other) requires(std::convertible_to<Y*, T*>) {
        this->counter = static_cast<SharedCounter<T>*>(other.counter);

        if (this->counter)
            this->counter->addWeakCount();
    }

    template <class Y>
    [[nodiscard]] explicit WeakPtr(const WeakPtr<Y>& other) requires(std::convertible_to<Y*, T*>) {
        this->counter = static_cast<SharedCounter<T>*>(other.counter);

        if (this->counter)
            this->counter->addWeakCount();
    }

    template <class Y>
    [[nodiscard]] explicit WeakPtr(WeakPtr<Y>&& other) requires(std::convertible_to<Y*, T*>) {
        this->counter = static_cast<SharedCounter<T>*>(other.counter);
        other.counter = nullptr;
    }

    ~WeakPtr() {
        if (this->counter)
            this->counter->releaseWeak();
    }

    template <class Y>
    WeakPtr& operator=(const SharedPtr<Y>& other) requires(std::convertible_to<Y*, T*>) {
        if (this->counter != static_cast<SharedCounter<T>*>(other.counter)) {
            this->counter = static_cast<SharedCounter<T>*>(other.counter);

            if (this->counter)
                this->counter->addWeakCount();
        }

        return *this;
    }

    template <class Y>
    WeakPtr& operator=(const WeakPtr<Y>& other) requires(std::convertible_to<Y*, T*>) {
        if (this->counter != static_cast<SharedCounter<T>*>(other.counter)) {
            this->counter = static_cast<SharedCounter<T>*>(other.counter);

            if (this->counter)
                this->counter->addWeakCount();
        }

        return *this;
    }

    template <class Y>
    WeakPtr& operator=(WeakPtr<Y>&& other) requires(std::convertible_to<Y*, T*>) {
        if (this->counter != static_cast<SharedCounter<T>*>(other.counter)) {
            this->counter = static_cast<SharedCounter<T>*>(other.counter);
            other.counter = nullptr;
        }

        return *this;
    }

    [[nodiscard]] int use_count() const {
        return this->counter ? this->counter->getShareCount() : 0;
    }

    [[nodiscard]] bool expired() const {
        return this->use_count() == 0;
    }

    [[nodiscard]] SharedPtr<T> lock() const {
        return this->expired() ? SharedPtr<T>() : SharedPtr<T>(*this);
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

    SharedCounter<T>* counter;
};
