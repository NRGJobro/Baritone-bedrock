#pragma once

#include "../../../Baritone/BaritoneController.h"
#include "../Module.h"

class BaritoneModule final : public Module {
    baritone::BaritoneController controller{};

    void reply(const std::string& text) const;

public:
    BaritoneModule();

    std::string getName() override;
    void onEnable() override;
    void onDisable() override;
    void onTick() override;
    void onPostTick() override;
    void onBeforeRenderLevel() override;
    void onAfterRenderLevel() override;
    void onRenderLevel() override;

    bool handleChat(const std::string& message);
    [[nodiscard]] baritone::BaritoneController& getController();
    [[nodiscard]] const baritone::BaritoneController& getController() const;
};
