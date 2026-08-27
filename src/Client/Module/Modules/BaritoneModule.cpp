#include "BaritoneModule.h"

#include "../../../SDK/MC.h"

#include <charconv>
#include <sstream>
#include <unordered_set>

namespace {

std::vector<std::string> tokenize(const std::string& input) {
    std::istringstream stream(input);
    std::vector<std::string> result;
    for (std::string token; stream >> token;)
        result.push_back(std::move(token));
    return result;
}

bool parseInt(const std::string& value, int& result) {
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    return error == std::errc{} && end == value.data() + value.size();
}

std::string lower(std::string value) {
    std::ranges::transform(value, value.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

} // namespace

BaritoneModule::BaritoneModule() : Module("Baritone pathfinding for Minecraft Bedrock") {}

std::string BaritoneModule::getName() { return "Baritone"; }

void BaritoneModule::onEnable() {
    reply("Enabled. Use .help for commands.");
}

void BaritoneModule::onDisable() {
    controller.stop();
}

void BaritoneModule::onTick() {
    controller.tick();
    if (controller.getState() == baritone::ControllerState::Arrived) {
        setEnabled(false);
    }
}

void BaritoneModule::onRenderLevel() {
    controller.render();
}

bool BaritoneModule::handleChat(const std::string& message) {
    std::string commandLine;
    bool directDotCommand = false;
    if (message.starts_with(".b "))
        commandLine = message.substr(3);
    else if (message == ".b")
        commandLine = "help";
    else if (message.starts_with('.') && message.size() > 1) {
        commandLine = message.substr(1);
        directDotCommand = true;
    }
    else
        return false;

    auto args = tokenize(commandLine);
    if (args.empty())
        args.emplace_back("help");

    const auto command = lower(args[0]);

    // Only consume direct dot commands that belong to Baritone. This lets
    // server commands such as .warp and .home continue to reach the server.
    static const std::unordered_set<std::string> commands{
        "help", "goto", "goal", "path", "stop", "cancel", "pause",
        "resume", "status", "water", "fall", "parkour", "y", "xz", "near"
    };
    if (directDotCommand && !commands.contains(command))
        return false;

    if (command == "help") {
        reply(".goto x y z | .goal x y z | .path | .stop");
        reply(".xz x z | .y level | .near x y z radius");
        reply(".pause | .resume | .status | .water on/off | .parkour on/off | .fall 0-20");
        return true;
    }

    if (command == "stop" || command == "cancel") {
        controller.stop();
        reply("Stopped.");
        return true;
    }

    if (command == "pause") {
        controller.pause();
        reply("Paused.");
        return true;
    }

    if (command == "resume") {
        controller.resume();
        reply("Resumed.");
        return true;
    }

    if (command == "status") {
        reply(controller.getStatusLine());
        return true;
    }

    if (command == "path") {
        if (!isEnabled())
            setEnabled(true);
        if (!controller.path())
            reply("Cannot path: set a goal and join a world first.");
        else
            reply("Calculating path.");
        return true;
    }

    if (command == "water" && args.size() == 2) {
        const auto value = lower(args[1]);
        if (value != "on" && value != "off") {
            reply("Usage: .water on/off");
            return true;
        }
        controller.getOptions().allowWater = value == "on";
        reply("Water traversal " + value + ".");
        return true;
    }

    if (command == "fall" && args.size() == 2) {
        int height{};
        if (!parseInt(args[1], height) || height < 0 || height > 20) {
            reply("Usage: .fall 0-20");
            return true;
        }
        controller.getOptions().allowFall = height > 0;
        controller.getOptions().maxFallHeight = height;
        reply("Maximum fall height set to " + std::to_string(height) + ".");
        return true;
    }

    if (command == "parkour" && args.size() == 2) {
        const auto value = lower(args[1]);
        if (value != "on" && value != "off") {
            reply("Usage: .parkour on/off");
            return true;
        }
        controller.getOptions().allowParkour = value == "on";
        reply("Parkour " + value + ".");
        return true;
    }

    if (command == "y" && args.size() == 2) {
        int y{};
        if (!parseInt(args[1], y)) {
            reply("Usage: .y level");
            return true;
        }
        if (!isEnabled())
            setEnabled(true);
        controller.goTo(std::make_shared<baritone::GoalYLevel>(y));
        return true;
    }

    if (command == "xz" && args.size() == 3) {
        int x{}, z{};
        if (!parseInt(args[1], x) || !parseInt(args[2], z)) {
            reply("Usage: .xz x z");
            return true;
        }
        if (!isEnabled())
            setEnabled(true);
        controller.goTo(std::make_shared<baritone::GoalXZ>(x, z));
        return true;
    }

    if (command == "near" && args.size() == 5) {
        int x{}, y{}, z{}, radius{};
        if (!parseInt(args[1], x) || !parseInt(args[2], y) || !parseInt(args[3], z) || !parseInt(args[4], radius) || radius < 0) {
            reply("Usage: .near x y z radius");
            return true;
        }
        if (!isEnabled())
            setEnabled(true);
        controller.goTo(std::make_shared<baritone::GoalNear>(baritone::BlockPos{x, y, z}, radius));
        return true;
    }

    if ((command == "goto" || command == "goal") && args.size() == 4) {
        int x{}, y{}, z{};
        if (!parseInt(args[1], x) || !parseInt(args[2], y) || !parseInt(args[3], z)) {
            reply("Usage: ." + command + " x y z");
            return true;
        }

        auto goal = std::make_shared<baritone::GoalBlock>(baritone::BlockPos{x, y, z});
        if (!isEnabled())
            setEnabled(true);

        if (command == "goal") {
            controller.setGoal(std::move(goal));
            reply("Goal set. Use .path to begin.");
        } else if (!controller.goTo(std::move(goal))) {
            reply("Join a world before starting pathing.");
        } else {
            reply("Calculating path.");
        }
        return true;
    }

    reply("Unknown or malformed command. Use .help.");
    return true;
}

const baritone::BaritoneController& BaritoneModule::getController() const { return controller; }

baritone::BaritoneController& BaritoneModule::getController() { return controller; }

void BaritoneModule::reply(const std::string& text) const {
    if (const auto gui = MC::getGuiData())
        gui->displayClientMessage("\xC2\xA7" "6[Baritone]" "\xC2\xA7" "r " + text);
}
