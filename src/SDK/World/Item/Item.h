#pragma once

#include <string>

class Item {
public:
    [[nodiscard]] const std::string& getRawName() const {
        return *reinterpret_cast<const std::string*>(
            reinterpret_cast<const std::byte*>(this) + 0xD8);
    }
};
