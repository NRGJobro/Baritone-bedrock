#pragma once

enum class ContainerID : int8_t {
    None = -1,
    inventory = 0,
    First = 1,
    Last = 100,
    Offhand = 119,
    Armor = 120,
    SelectionSlots = 122, // Hotbar
    PlayerUIOnly = 124,
    Registry = 125
};
