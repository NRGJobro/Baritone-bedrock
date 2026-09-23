#pragma once

#include "../../../Baritone/BaritoneController.h"
#include "../../../Baritone/Bedrock/ElytraProcess.h"
#include "../../../Baritone/Bedrock/ExploreProcess.h"
#include "../../../Baritone/Bedrock/MiningProcess.h"
#include "../Module.h"

#include <unordered_map>

class BaritoneModule final : public Module {
    baritone::BaritoneController controller{};
    baritone::MiningProcess miningProcess{};
    baritone::ExploreProcess exploreProcess{};
    baritone::ElytraProcess elytraProcess{};
    std::unordered_map<std::string, baritone::BlockPos> waypoints;

    void reply(const std::string& text) const;
    void stopProcesses();

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
