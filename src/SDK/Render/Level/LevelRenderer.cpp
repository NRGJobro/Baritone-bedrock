#include "LevelRenderer.h"

#include "LevelRendererPlayer.h"

LevelRendererPlayer* LevelRenderer::getLevelRendererPlayer() {
    return hat::member_at<LevelRendererPlayer*>(this, 0x468);
}

const glm::vec3& LevelRenderer::getCameraPos() {
    static const glm::vec3 fallback{};
    auto* playerRenderer = this->getLevelRendererPlayer();
    return playerRenderer == nullptr ? fallback : playerRenderer->getCameraPos();
}
