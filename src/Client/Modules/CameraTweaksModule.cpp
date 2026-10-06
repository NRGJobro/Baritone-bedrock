#include "CameraTweaksModule.h"

#include "../../Client.h"
#include "../../SDK/Client/Camera/CameraComponent.h"
#include "../../SDK/MC.h"
#include "../../SDK/World/Actor/LocalPlayer.h"
#include "../../SDK/World/Actor/Components/RenderPositionComponent.h"
#include "CameraTweaksMath.h"

CameraTweaksModule::CameraTweaksModule() : Module("Hold Ctrl and scroll to move the third-person camera closer or farther from your player") {}

std::string CameraTweaksModule::getName() {
    return "CameraTweaks";
}

void CameraTweaksModule::onEnable() {
    cameraActive = false;
}

void CameraTweaksModule::onDisable() {
    cameraActive = false;
}

void CameraTweaksModule::setPerspective(const int value) {
    perspective.store(value, std::memory_order_relaxed);
}

int CameraTweaksModule::getPerspective() const {
    return perspective.load(std::memory_order_relaxed);
}

bool CameraTweaksModule::onWheel(const bool up, const bool controlHeld) {
    if (MC::getLocalPlayer() == nullptr || !CameraTweaksMath::acceptsScroll(
        getPerspective(),
        g_Client.hudScreenActive.load(std::memory_order_acquire),
        g_Client.clickGuiOpened,
        controlHeld))
        return false;

    distance = CameraTweaksMath::scrollDistance(distance, scrollStep, up);
    return true;
}

void CameraTweaksModule::apply(MinecraftCamera::CameraComponent* camera) {
    auto* player = MC::getLocalPlayer();
    const int view = getPerspective();
    if (camera == nullptr || player == nullptr || (view != 1 && view != 2)) {
        cameraActive = false;
        return;
    }

    const auto* renderPosition = player->tryGet<RenderPositionComponent>();
    if (renderPosition == nullptr) {
        cameraActive = false;
        return;
    }

    const glm::vec3 offset = camera->origin - renderPosition->renderPos;
    const float nativeDistance = glm::length(offset);
    if (!std::isfinite(nativeDistance) || nativeDistance < 0.01f) {
        cameraActive = false;
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const float deltaTime = cameraActive ? std::chrono::duration<float>(now - lastUpdate).count() : 0.f;
    if (!cameraActive)
        currentDistance = nativeDistance;
    lastUpdate = now;
    cameraActive = true;

    distance = std::clamp(std::isfinite(distance) ? distance : 4.f, 0.5f, 32.f);
    currentDistance = smooth ? CameraTweaksMath::approach(currentDistance, distance, deltaTime) : distance;
    camera->origin = renderPosition->renderPos + offset * (currentDistance / nativeDistance);
}
