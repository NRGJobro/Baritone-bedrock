#include "ServerInstance.h"

Minecraft* ServerInstance::getMinecraft() {
    return hat::member_at<Minecraft*>(this, 0xB8);
}
