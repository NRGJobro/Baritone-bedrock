#pragma once

// Custom class
struct MapBrightness {
    enum Value : uint8_t {
        Low,
        Normal,
        High,
        Lowest
    };

    static uint8_t getModifier(const Value brightness) {
        switch (brightness) {
            case Low:
                return 180;
            case Normal:
                return 220;
            case High:
                return 255;
            case Lowest:
                return 135;
            default:
                return 220;
        }
    }
};
