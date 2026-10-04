#pragma once

#include "MaterialType.h"

#include <cstddef>

class Material {
public:
    MaterialType type;
    bool isReplaceable;

    bool isType(const MaterialType material) const {
        return this->type == material;
    }
};
