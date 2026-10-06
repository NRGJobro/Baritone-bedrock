#include "VisualTree.h"

#include "UIControl.h"

UIControl* VisualTree::getRootControl() {
    return hat::member_at<UIControl*>(this, 0x8);
}
