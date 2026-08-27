#pragma once

#include <cstdint>

// Bedrock stores MaterialType as a 32-bit value. Using an 8-bit underlying
// type shifts every bool in Material and makes ordinary floors look non-solid.
enum class MaterialType : std::uint32_t {
    Air,
    Dirt,
    Wood,
    Metal,
    Grate,
    Water,
    Lava,
    Leaves,
    Plant,
    SolidPlant,
    Fire,
    Glass,
    Explosive,
    Ice,
    PowderSnow,
    Cactus,
    Portal,
    StoneDecoration,
    Bubble,
    Barrier,
    DecorationSolid,
    ClientRequestPlaceholder,
    StructureVoid,
    Solid,
    NonSolid,
    Any
};
