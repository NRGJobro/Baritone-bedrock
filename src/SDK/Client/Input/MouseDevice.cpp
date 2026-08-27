#include "MouseDevice.h"

#include "../../../Memory/Sig/SignatureManager.h"
#include "../../../Utils/Utils.h"
#include "../../MC.h"

void MouseDevice::feed(char actionButtonId, char isDown, short mouseX, short mouseY, short relativeMovementX, short relativeMovementY, bool forceMotionlessPointer) {
    static auto sig = GET_SIG("MouseDevice::feed");
    static auto feed = *(decltype(&MouseDevice::feed)*)&sig;
    (this->*feed)(actionButtonId, isDown, mouseX, mouseY, relativeMovementX, relativeMovementY, forceMotionlessPointer);
}

void MouseDevice::fixMouse() {
    static auto mouse = get();

    const auto gui = MC::getGuiData();

    const auto oldX = gui->mouseX, oldY = gui->mouseY;

    mouse->feed(0, 0, oldX + 1, oldY + 1, 0, 0, false);
    mouse->feed(0, 0, oldX, oldY, 0, 0, false);
}

MouseDevice* MouseDevice::get() {
    static auto mouse = Utils::getFromOffset<MouseDevice*>(GET_SIG("MouseDevice::_instance"), 2);
    return mouse;
}
