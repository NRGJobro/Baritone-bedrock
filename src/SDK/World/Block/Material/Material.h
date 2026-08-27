#pragma once

#include "MaterialType.h"

#include <cstddef>

class Material {
public:
    enum class Settings : int {
        Solid,
        Liquid,
        NonSolid
    };

    MaterialType type;
    bool neverBuildable;
    bool liquid;
    bool blocksMotion;
    bool blocksPrecipitation;
    bool solid;
    bool superHot;

    bool isType(const MaterialType material) const {
        return this->type == material;
    }
};

static_assert(offsetof(Material, neverBuildable) == 0x4);
static_assert(offsetof(Material, liquid) == 0x5);
static_assert(offsetof(Material, blocksMotion) == 0x6);
static_assert(offsetof(Material, solid) == 0x8);
static_assert(offsetof(Material, superHot) == 0x9);
