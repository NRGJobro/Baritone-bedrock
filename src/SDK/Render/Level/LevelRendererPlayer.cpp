#include "LevelRendererPlayer.h"

const glm::vec3& LevelRendererPlayer::getCameraPos() {
    return hat::member_at<glm::vec3>(this, 0x660);
}
