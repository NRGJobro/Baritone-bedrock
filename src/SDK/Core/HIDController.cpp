#include "HIDController.h"

HWND HIDController::getWindowHandle() {
    return hat::member_at<HWND>(this, 0xC8);
}
