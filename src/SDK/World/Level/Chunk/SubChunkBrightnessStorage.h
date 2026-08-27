#pragma once

#include "../Brightness.h"

class SubChunkBrightnessStorage {
public:
    struct LightPair {
        union {
            struct {
                uint8_t blockLight : 4;
                uint8_t skyLight   : 4;
            };
            uint8_t raw;
        };
    };

    std::array<Brightness, 2048> lightValues;

    // Custom function
    Brightness getLightLevelAt(glm::ivec3 localPos) const;
};
