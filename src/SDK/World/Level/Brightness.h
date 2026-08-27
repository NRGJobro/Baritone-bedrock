#pragma once

struct Brightness {
    uint8_t value = 0;

    static Brightness MAX() { return Brightness(std::numeric_limits<uint8_t>::max()); }
    static Brightness MIN() { return Brightness(std::numeric_limits<uint8_t>::min()); }
};
