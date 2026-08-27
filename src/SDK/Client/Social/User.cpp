#include "User.h"

void* Social::User::getOptions() {
    return hat::member_at<void*>(this, 0x3C8);
}
