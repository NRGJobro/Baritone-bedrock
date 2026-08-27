#include "MainWindow.h"

MinecraftGame* MainWindow::getMinecraftGame() {
    return hat::member_at<MinecraftGame*>(this, 0x38);
}

HIDController* MainWindow::getHIDController() {
    return hat::member_at<HIDController*>(this, 0x98);
}
