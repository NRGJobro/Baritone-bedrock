#pragma once

#include "Module.h"

namespace MinecraftCamera { struct CameraComponent; }

class CameraTweaksModule final : public Module {
    float distance = 4.f;
    float scrollStep = 0.5f;
    bool smooth = true;
    float currentDistance = 4.f;
    bool cameraActive = false;
    std::chrono::steady_clock::time_point lastUpdate{};
    std::atomic_int perspective{0};

public:
    CameraTweaksModule();

    std::string getName() override;
    void onEnable() override;
    void onDisable() override;

    void setPerspective(int value);
    [[nodiscard]] int getPerspective() const;
    bool onWheel(bool up);
    void apply(MinecraftCamera::CameraComponent* camera);
};
