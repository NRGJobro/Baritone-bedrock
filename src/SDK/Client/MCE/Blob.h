#pragma once

namespace mce {
    class Blob {
    public:
        using value_type     = uint8_t;
        using size_type      = size_t;
        using pointer        = value_type*;
        using iterator       = value_type*;
        using const_pointer  = value_type const*;
        using const_iterator = value_type const*;

        using delete_function = void (*)(pointer);

        struct Deleter {
            delete_function fn;

            [[nodiscard]] _CONSTEXPR23 Deleter() : fn(defaultDeleter) {}

            [[nodiscard]] _CONSTEXPR23 Deleter(delete_function fn) : fn(fn) {}

            void operator()(pointer x) const { fn(x); }
        };

        using pointer_type = std::unique_ptr<value_type[], Deleter>;

        pointer_type blob; // 0x0
        size_type    size; // 0x10

        [[nodiscard]] inline pointer getData() const { return blob.get(); }

        [[nodiscard]] inline size_t getSize() const { return size; }

        Blob() = default;
        Blob(const value_type* data, size_t size);
        Blob(const Blob& other);

        Blob& operator=(const Blob& other);

        static void defaultDeleter(pointer p);
    };
}
