#include "VisualTree.h"

UIControl* VisualTree::getRootControl() {
    return hat::member_at<UIControl*>(this, 0x8);
}
