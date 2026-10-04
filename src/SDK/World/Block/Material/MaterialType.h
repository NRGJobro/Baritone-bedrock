#pragma once

#include <cstdint>

enum class MaterialType : std::uint32_t {
    Air, Dirt, Wood, Stone, Metal, Water, Lava, Leaves, Plant,
    ReplaceablePlant, Sponge, Cloth, Bed, Fire, Sand, Decoration,
    Glass, Explosive, Ice, PackedIce, TopSnow, Snow, PowderSnow, Unknown23,
    Cactus, Clay, Vegetable, Portal, Cake, Web, RedstoneWire, Carpet,
    BuildableGlass, Slime, Piston, Allow, Deny, Netherwart,
    StoneDecoration, Bubble, Egg, Barrier, DecorationFlammable,
    SurfaceTypeTotal, Any
};
