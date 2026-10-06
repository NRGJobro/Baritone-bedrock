#pragma once

#include "../../Util/HashedString.h"
#include "../../World/Actor/Components/IEntityComponent.h"

namespace MinecraftCamera {

class CameraComponent : public IEntityComponent {
public:
    HashedString viewName{};
    glm::quat rotation{};
    glm::vec3 origin{};
    glm::vec4 fov{};
    glm::mat4 world{};
    glm::mat4 view{};
    glm::mat4 projection{};
    std::int8_t padding[4]{};
};

static_assert(sizeof(CameraComponent) == 0x120);

} // namespace MinecraftCamera
