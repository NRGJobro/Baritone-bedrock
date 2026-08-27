#include "CameraAccessUtils.h"

#include "../../World/Actor/Components/RenderCameraComponent.h"

MinecraftCamera::CameraComponent* CameraAccessUtils::getCurrentRenderCameraComponent(ClientInstance* clientInstance) {
    auto& registry = clientInstance->getMinecraft()->getEntityRegistry()->ownedRegistry;
    const auto& storage = registry.storage<MinecraftCamera::RenderCameraComponent>();
    const auto entityId = storage[0];

    return registry.try_get<MinecraftCamera::CameraComponent>(entityId);
}
