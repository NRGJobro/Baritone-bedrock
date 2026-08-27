#pragma once

#include "LevelRendererPlayer.h"

class LevelRenderer {
public:
    LevelRendererPlayer* getLevelRendererPlayer();

    const glm::vec3& getCameraPos();
};
