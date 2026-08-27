#pragma once

struct MapDecoration {
    enum class Type : int8_t {
        MarkerWhite = 0,
        MarkerGreen = 1,
        MarkerRed = 2,
        MarkerBlue = 3,
        XWhite = 4,
        TriangleRed = 5,
        SquareWhite = 6,
        MarkerSign = 7,
        MarkerPink = 8,
        MarkerOrange = 9,
        MarkerYellow = 10,
        MarkerTeal = 11,
        TriangleGreen = 12,
        SmallSquareWhite = 13,
        Mansion = 14,
        Monument = 15,
        NoDraw = 16,
        VillageDesert = 17,
        VillagePlains = 18,
        VillageSavanna = 19,
        VillageSnowy = 20,
        VillageTaiga = 21,
        JungleTemple = 22,
        WitchHut = 23,
        TrialChambers = 24,
        Count = 25,
        Player = 0,
        PlayerOffMap = 6,
        PlayerOffLimits = 13,
        PlayerHidden = 16,
        ItemFrame = 1
    };

    std::string label;
    Type image;
    char x;
    char y;
    char rotation;
    mce::Color color;
};
