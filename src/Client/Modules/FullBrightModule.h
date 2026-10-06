#pragma once

#include "Module.h"

class FullBrightModule final : public Module {
    float intensity = 25.f;

public:
    FullBrightModule();

    std::string getName() override;
    [[nodiscard]] float getIntensity() const;
};
