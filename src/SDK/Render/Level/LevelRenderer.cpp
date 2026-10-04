#include "LevelRenderer.h"

LevelRendererPlayer* LevelRenderer::getLevelRendererPlayer() {
    return hat::member_at<LevelRendererPlayer*>(this, 0x468);
}

const glm::vec3& LevelRenderer::getCameraPos() {
    return this->getLevelRendererPlayer()->getCameraPos();
}
