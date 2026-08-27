#pragma once

#include "MouseAction.h"

class MouseDevice {
public:
    int16_t clickX;
    int16_t clickY;
    int16_t x;
    int16_t y;
    int16_t dx;
    int16_t dy;
    int16_t xOld;
    int16_t yOld;
    bool buttonStates[7];
    std::vector<MouseAction> inputs;
    int32_t firstMovementType;

    void feed(char actionButtonId, char isDown, short mouseX, short mouseY, short relativeMovementX, short relativeMovementY, bool forceMotionlessPointer);

    static void fixMouse();

    static MouseDevice* get();
};
