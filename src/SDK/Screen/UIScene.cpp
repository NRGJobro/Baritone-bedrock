#include "UIScene.h"

ScreenView* UIScene::getScreenView() {
    return hat::member_at<ScreenView*>(this, 0x40);
}
