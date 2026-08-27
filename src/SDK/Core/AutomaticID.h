#pragma once

template<typename A, typename T>
class AutomaticID {
public:
    T id;

    using Type = A;

    [[nodiscard]] constexpr AutomaticID() : id(0) {}

    [[nodiscard]] constexpr AutomaticID(T x) : id(x) {}

    [[nodiscard]] constexpr operator T() const {
        return id;
    }

    [[nodiscard]] bool operator==(const AutomaticID& other) const {
        return id == other.id;
    }

    [[nodiscard]] bool operator==(const T& other) const {
        return id == other;
    }

    [[nodiscard]] std::strong_ordering operator<=>(const AutomaticID& other) const {
        return id <=> other.id;
    }

    [[nodiscard]] std::strong_ordering operator<=>(const T& other) const {
        return id <=> other;
    }
};

namespace std {
    template<typename A, typename T>
    class hash<AutomaticID<A, T>> {
    public:
        size_t operator()(const AutomaticID<A, T>& dimId) const {
            return static_cast<size_t>(dimId.id);
        }
    };
}
