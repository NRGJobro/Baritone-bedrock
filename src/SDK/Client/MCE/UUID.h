#pragma once

namespace mce {
    struct UUID {
        uint64_t mostSignificant;
        uint64_t leastSignificant;

        bool operator==(const UUID &other) const { return mostSignificant == other.mostSignificant && leastSignificant == other.leastSignificant; }

        UUID& operator=(const UUID& other) = default;

        bool operator<(const UUID& other) const {
            return this->mostSignificant < other.mostSignificant || this->mostSignificant <= other.mostSignificant && this->leastSignificant < other.leastSignificant;
        }

        std::string toString() const {
            std::stringstream ss;
            ss << std::hex << std::setfill('0');

            ss << std::setw(8) << static_cast<uint32_t>(this->mostSignificant >> 32);
            ss << "-";
            ss << std::setw(4) << static_cast<uint16_t>(this->mostSignificant >> 16 & 0xFFFF);
            ss << "-";
            ss << std::setw(4) << static_cast<uint16_t>(this->mostSignificant & 0xFFFF);
            ss << "-";

            ss << std::setw(4) << static_cast<uint16_t>(this->leastSignificant >> 48);
            ss << "-";
            ss << std::setw(4) << static_cast<uint16_t>(this->leastSignificant >> 32 & 0xFFFF);
            ss << std::setw(8) << static_cast<uint32_t>(this->leastSignificant & 0xFFFFFFFF);

            return ss.str();
        }
    };
}

namespace std {
    template <>
    struct hash<mce::UUID> {
        size_t operator()(mce::UUID const& id) const noexcept { return id.mostSignificant ^ (522133279 * id.leastSignificant); }
    };
}
