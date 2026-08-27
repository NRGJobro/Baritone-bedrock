#pragma once

class ClipUtils {
public:
    static bool canSee(const glm::vec3& start, int maxDistance, class Actor* actor, bool feet, bool head);
};
