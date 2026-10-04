#include "ScreenView.h"

VisualTree* ScreenView::getVisualTree() {
    return hat::member_at<VisualTree*>(this, 0x50);
}
