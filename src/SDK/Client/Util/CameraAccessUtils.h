#pragma once

#include "../../World/Actor/Components/CameraComponent.h"
#include "../ClientInstance.h"

namespace CameraAccessUtils {
    MinecraftCamera::CameraComponent* getCurrentRenderCameraComponent(ClientInstance* clientInstance);
}
