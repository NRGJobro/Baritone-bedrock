#pragma once

class LevelRendererPlayer;

class LevelRenderer {
public:
    LevelRendererPlayer* getLevelRendererPlayer();

    const glm::vec3& getCameraPos();
};
