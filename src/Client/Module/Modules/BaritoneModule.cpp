#include "BaritoneModule.h"

#include "../../../SDK/MC.h"
#include "../../../Baritone/Core/AdvancedGoals.h"
#include "../../../Baritone/Bedrock/BedrockWorld.h"
#include "../../../Baritone/Core/Movement.h"

#include <charconv>
#include <cmath>
#include <format>
#include <numbers>
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

bool parseDouble(const std::string& value, double& result) {
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    return error == std::errc{} && end == value.data() + value.size() && std::isfinite(result);
}

std::string lower(std::string value) {
    std::ranges::transform(value, value.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool parseToggle(const std::string& value, bool& result) {
    const auto normalized = lower(value);
    if (normalized == "on" || normalized == "true" || normalized == "1") {
        result = true;
        return true;
    }
    if (normalized == "off" || normalized == "false" || normalized == "0") {
        result = false;
        return true;
    }
    return false;
}

baritone::BlockPos getPlayerBlock() {
    const auto player = MC::getLocalPlayer();
    if (player == nullptr)
        return {};
    const auto feet = player->getFeetPosition();
    return {static_cast<int>(std::floor(feet.x)), static_cast<int>(std::floor(feet.y + 0.1251f)),
        static_cast<int>(std::floor(feet.z))};
}

bool parseBlockFilters(const std::string& value, std::vector<int>& ids, std::vector<std::string>& names) {
    std::size_t start = 0;
    while (start < value.size()) {
        const auto comma = value.find(',', start);
        const auto token = value.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        int id{};
        if (parseInt(token, id)) {
            if (id <= 0) return false;
            ids.push_back(id);
        } else {
            if (token.empty()) return false;
            names.push_back(token);
        }
        if (comma == std::string::npos)
            break;
        start = comma + 1;
    }
    return !ids.empty() || !names.empty();
}

} // namespace

BaritoneModule::BaritoneModule() : Module("Limiter pathfinding for Minecraft Bedrock") {}

std::string BaritoneModule::getName() { return "Limiter"; }

void BaritoneModule::onEnable() {
    reply("Enabled. Use .help for commands.");
}

void BaritoneModule::onDisable() {
    stopProcesses();
}

void BaritoneModule::onTick() {
    controller.tick();
    miningProcess.tick(controller);
    exploreProcess.tick(controller);
    if (auto message = miningProcess.takeMessage())
        reply(*message);
    if (auto message = exploreProcess.takeMessage())
        reply(*message);
}

void BaritoneModule::onPostTick() {
    controller.postTick();
}

void BaritoneModule::onBeforeRenderLevel() {
    controller.beginVisualRotationRender();
}

void BaritoneModule::onAfterRenderLevel() {
    controller.endVisualRotationRender();
}

void BaritoneModule::onRenderLevel() {
    controller.render(miningProcess.getRenderTargets(), miningProcess.isActive());
}

bool BaritoneModule::handleChat(const std::string& message) {
    std::string commandLine;
    bool directDotCommand = false;
    if (message.starts_with(".l ")) commandLine = message.substr(3);
    else if (message == ".l") commandLine = "help";
    else if (message.starts_with(".b ")) commandLine = message.substr(3);
    else if (message == ".b") commandLine = "help";
    else if (message.starts_with('.') && message.size() > 1) {
        commandLine = message.substr(1);
        directDotCommand = true;
    } else return false;

    auto args = tokenize(commandLine);
    if (args.empty()) args.emplace_back("help");
    const auto command = lower(args[0]);

    // Leave unrelated direct-dot commands available to servers and other
    // clients. The .b/.l namespace always belongs to Limiter.
    static const std::unordered_set<std::string> commands{
        "help", "goto", "goal", "path", "stop", "cancel", "pause", "resume",
        "status", "eta", "water", "fall", "parkour", "bridge", "set", "y", "xz",
        "near", "interact", "twoblocks", "axis", "highway", "thisway", "forward",
        "away", "surface", "top", "mine", "tunnel", "explore", "waypoint", "wp"
    };
    if (directDotCommand && !commands.contains(command)) return false;

    const auto enable = [&] { if (!isEnabled()) setEnabled(true); };
    const auto beginManualGoal = [&](std::shared_ptr<baritone::Goal> goal) {
        stopProcesses();
        enable();
        if (!controller.goTo(std::move(goal))) reply("Join a world before starting pathing.");
        else reply("Calculating path.");
    };

    if (command == "help") {
        reply("NAV: .goto/.goal x y z | .xz x z | .y level | .near x y z r");
        reply("GOALS: .axis | .thisway blocks | .away x y z blocks | .surface");
        reply("PROCESS: .mine name[,name] [count] [radius] [continue] | .tunnel [h w depth] | .explore [chunkRadius]");
        reply("WAYPOINTS: .wp save/goto/delete/list <name>");
        reply("CONTROL: .path | .stop | .pause | .resume | .status | .eta");
        reply("POLICY: .set name value | .water | .fall | .parkour | .bridge");
        return true;
    }

    if (command == "stop" || command == "cancel") {
        stopProcesses();
        reply("Stopped all Limiter processes.");
        return true;
    }
    if (command == "pause") { controller.pause(); reply("Paused."); return true; }
    if (command == "resume") { controller.resume(); reply("Resumed."); return true; }

    if (command == "status") {
        reply(controller.getStatusLine());
        if (miningProcess.isActive()) reply(miningProcess.getStatusLine());
        if (exploreProcess.isActive()) reply(exploreProcess.getStatusLine());
        reply("waypoints: " + std::to_string(waypoints.size()));
        return true;
    }
    if (command == "eta") {
        const double ticks = controller.getEstimatedTicksToGoal();
        if (ticks <= 0.0) reply("No active path ETA is available.");
        else reply(std::format("Estimated remaining: {:.1f}s ({:.0f} ticks).", ticks / 20.0, ticks));
        return true;
    }
    if (command == "path") {
        stopProcesses();
        enable();
        reply(controller.path() ? "Calculating path." : "Cannot path: set a goal and join a world first.");
        return true;
    }

    // Compatibility shorthands for the most frequently changed policies.
    if ((command == "water" || command == "parkour" || command == "bridge") && args.size() == 2) {
        bool value{};
        if (!parseToggle(args[1], value)) { reply("Usage: ." + command + " on/off"); return true; }
        if (command == "water") controller.getOptions().allowWater = value;
        else if (command == "parkour") controller.getOptions().allowParkour = value;
        else controller.getOptions().allowBridge = value;
        reply(command + std::string(value ? " enabled." : " disabled."));
        return true;
    }
    if (command == "fall" && args.size() == 2) {
        int height{};
        if (!parseInt(args[1], height) || height < 0 || height > 64) { reply("Usage: .fall 0-64"); return true; }
        controller.getOptions().allowFall = height > 0;
        controller.getOptions().maxFallHeight = height;
        reply("Maximum fall height set to " + std::to_string(height) + ".");
        return true;
    }

    if (command == "set") {
        auto& options = controller.getOptions();
        if (args.size() == 1) {
            reply(std::format("water={} diagonal={} ascend={} fall={} parkour={} bridge={} sprint={}",
                options.allowWater, options.allowDiagonal, options.allowAscend, options.maxFallHeight,
                options.allowParkour, options.allowBridge, controller.getExecutionOptions().sprint));
            reply(std::format("nodesPerTick={} maxNodes={} parkourDistance={} bridgeLength={}",
                options.nodesPerTick, options.maxExpandedNodes, options.maxParkourDistance, options.maxBridgeLength));
            return true;
        }
        if (args.size() != 3) { reply("Usage: .set <setting> <value>"); return true; }
        const auto name = lower(args[1]);
        bool toggle{};
        int number{};
        if (name == "water" && parseToggle(args[2], toggle)) options.allowWater = toggle;
        else if (name == "diagonal" && parseToggle(args[2], toggle)) options.allowDiagonal = toggle;
        else if ((name == "ascend" || name == "step") && parseToggle(args[2], toggle)) options.allowAscend = toggle;
        else if (name == "parkour" && parseToggle(args[2], toggle)) options.allowParkour = toggle;
        else if (name == "parkourascend" && parseToggle(args[2], toggle)) options.allowParkourAscend = toggle;
        else if (name == "bridge" && parseToggle(args[2], toggle)) options.allowBridge = toggle;
        else if (name == "sprint" && parseToggle(args[2], toggle)) controller.getExecutionOptions().sprint = toggle;
        else if ((name == "autoreplan" || name == "replan") && parseToggle(args[2], toggle)) controller.getReplanWhenStuck() = toggle;
        else if (name == "nodespertick" && parseInt(args[2], number) && number >= 25 && number <= 10000) options.nodesPerTick = number;
        else if (name == "maxnodes" && parseInt(args[2], number) && number >= 1000 && number <= 1000000) options.maxExpandedNodes = number;
        else if (name == "fallheight" && parseInt(args[2], number) && number >= 0 && number <= 64) {
            options.maxFallHeight = number; options.allowFall = number > 0;
        }
        else if (name == "parkourdistance" && parseInt(args[2], number) && number >= 2 && number <= 4) options.maxParkourDistance = number;
        else if (name == "bridgelength" && parseInt(args[2], number) && number >= 1 && number <= 16) options.maxBridgeLength = number;
        else { reply("Unknown setting or invalid value. Use .set to list settings."); return true; }
        reply("Set " + name + " to " + args[2] + ".");
        return true;
    }

    if (command == "mine") {
        if (args.size() == 2 && lower(args[1]) == "stop") {
            miningProcess.cancel(controller); reply("Mining stopped."); return true;
        }
        if (args.size() < 2 || args.size() > 5) {
            reply("Usage: .mine id[,id] [count; 0=continuous] [scanRadius] [continue]"); return true;
        }
        std::vector<int> ids;
        std::vector<std::string> names;
        int quantity = 0;
        int radius = 24;
        bool continueMode = false;
        bool quantitySeen = false;
        bool radiusSeen = false;
        bool valid = parseBlockFilters(args[1], ids, names);
        for (std::size_t index = 2; valid && index < args.size(); ++index) {
            const auto token = lower(args[index]);
            if (token == "continue" || token == "continuous" || token == "loop") {
                continueMode = true;
                continue;
            }
            int value{};
            if (!quantitySeen && parseInt(token, value) && value >= 0) {
                quantity = value;
                quantitySeen = true;
            } else if (!radiusSeen && parseInt(token, value) && value >= 4 && value <= 64) {
                radius = value;
                radiusSeen = true;
            } else {
                valid = false;
            }
        }
        if (!valid || (args.size() > 2 && !quantitySeen && !continueMode) ||
            (args.size() > 3 && !radiusSeen && !continueMode)) {
            reply("Usage: .mine id[,id] [count; 0=continuous] [scanRadius 4-64] [continue]"); return true;
        }
        stopProcesses();
        miningProcess.start(std::move(ids), std::move(names), quantity, radius, continueMode);
        enable();
        return true;
    }

    if (command == "tunnel") {
        int height = 2;
        int width = 1;
        int depth = 32;
        if (args.size() != 1 && args.size() != 4) {
            reply("Usage: .tunnel [height width depth]"); return true;
        }
        if (args.size() == 4 && (!parseInt(args[1], height) || !parseInt(args[2], width) ||
            !parseInt(args[3], depth) || height < 2 || height > 8 || width < 1 || width > 9 ||
            depth < 1 || depth > 128)) {
            reply("Tunnel limits: height 2-8, width 1-9, depth 1-128."); return true;
        }
        const auto player = MC::getLocalPlayer();
        if (player == nullptr) { reply("Join a world before tunneling."); return true; }
        const auto start = getPlayerBlock();
        const double yaw = static_cast<double>(player->getRotation().y) * std::numbers::pi / 180.0;
        int forwardX = static_cast<int>(std::round(-std::sin(yaw)));
        int forwardZ = static_cast<int>(std::round(std::cos(yaw)));
        if (std::abs(forwardX) > std::abs(forwardZ)) forwardZ = 0;
        else forwardX = 0;
        const int sideX = -forwardZ;
        const int sideZ = forwardX;
        const int firstSide = -(width / 2);
        std::vector<baritone::BlockPos> blocks;
        blocks.reserve(static_cast<std::size_t>(height * width * depth));
        for (int step = 1; step <= depth; ++step) {
            for (int side = 0; side < width; ++side) {
                const int lateral = firstSide + side;
                for (int y = 0; y < height; ++y) {
                    blocks.push_back({start.x + forwardX * step + sideX * lateral, start.y + y,
                        start.z + forwardZ * step + sideZ * lateral});
                }
            }
        }
        stopProcesses();
        miningProcess.startTargets(std::move(blocks));
        enable();
        return true;
    }

    if (command == "explore") {
        if (args.size() == 2 && lower(args[1]) == "stop") {
            exploreProcess.cancel(controller); reply("Exploration stopped."); return true;
        }
        int radius = 0;
        if (args.size() > 2 || (args.size() == 2 && (!parseInt(args[1], radius) || radius < 0 || radius > 256))) {
            reply("Usage: .explore [chunkRadius; 0=continuous]"); return true;
        }
        if (MC::getLocalPlayer() == nullptr) { reply("Join a world before exploring."); return true; }
        stopProcesses();
        exploreProcess.start(getPlayerBlock(), radius);
        enable();
        return true;
    }

    if (command == "waypoint" || command == "wp") {
        if (args.size() == 2 && lower(args[1]) == "list") {
            if (waypoints.empty()) reply("No session waypoints saved.");
            for (const auto& [name, pos] : waypoints)
                reply(name + ": " + std::to_string(pos.x) + " " + std::to_string(pos.y) + " " + std::to_string(pos.z));
            return true;
        }
        if (args.size() != 3) { reply("Usage: .wp save/goto/delete <name> | .wp list"); return true; }
        const auto action = lower(args[1]);
        const auto name = lower(args[2]);
        if (action == "save") {
            if (MC::getLocalPlayer() == nullptr) { reply("Join a world before saving a waypoint."); return true; }
            waypoints[name] = getPlayerBlock();
            reply("Saved waypoint " + name + ".");
        } else if (action == "delete" || action == "remove") {
            reply(waypoints.erase(name) ? "Deleted waypoint " + name + "." : "Waypoint not found.");
        } else if (action == "goto") {
            const auto found = waypoints.find(name);
            if (found == waypoints.end()) reply("Waypoint not found.");
            else beginManualGoal(std::make_shared<baritone::GoalBlock>(found->second));
        } else reply("Usage: .wp save/goto/delete <name> | .wp list");
        return true;
    }

    if (command == "axis" || command == "highway") {
        beginManualGoal(std::make_shared<baritone::GoalAxis>());
        return true;
    }
    if ((command == "thisway" || command == "forward") && args.size() == 2) {
        double distance{};
        const auto player = MC::getLocalPlayer();
        if (!parseDouble(args[1], distance) || distance <= 0.0 || distance > 100000.0 || player == nullptr) {
            reply("Usage: .thisway <distance 1-100000>"); return true;
        }
        const auto start = getPlayerBlock();
        const double yaw = static_cast<double>(player->getRotation().y) * std::numbers::pi / 180.0;
        const int x = start.x + static_cast<int>(std::round(-std::sin(yaw) * distance));
        const int z = start.z + static_cast<int>(std::round(std::cos(yaw) * distance));
        beginManualGoal(std::make_shared<baritone::GoalXZ>(x, z));
        return true;
    }
    if (command == "away" && (args.size() == 5 || args.size() == 6)) {
        int x{}, y{}, z{}, distance{};
        bool maintainY = false;
        if (!parseInt(args[1], x) || !parseInt(args[2], y) || !parseInt(args[3], z) ||
            !parseInt(args[4], distance) || distance < 1 || (args.size() == 6 && !parseToggle(args[5], maintainY))) {
            reply("Usage: .away x y z distance [maintainY on/off]"); return true;
        }
        beginManualGoal(std::make_shared<baritone::GoalRunAway>(baritone::BlockPos{x, y, z}, distance, maintainY));
        return true;
    }

    if (command == "surface" || command == "top") {
        auto* region = MC::getRegion();
        if (region == nullptr) { reply("Join a world before finding the surface."); return true; }
        const auto start = getPlayerBlock();
        const baritone::BedrockWorld world(region);
        std::vector<baritone::BlockPos> candidates;
        for (int radius = 0; radius <= 16 && candidates.size() < 64; ++radius) {
            for (int dx = -radius; dx <= radius; ++dx) for (int dz = -radius; dz <= radius; ++dz) {
                if (radius > 0 && std::abs(dx) != radius && std::abs(dz) != radius) continue;
                for (int y = 320; y >= start.y; --y) {
                    const baritone::BlockPos candidate{start.x + dx, y, start.z + dz};
                    if (baritone::MovementGenerator::canStandAt(world, candidate, controller.getOptions())) {
                        candidates.push_back(candidate);
                        break;
                    }
                }
            }
        }
        if (candidates.empty()) { reply("No loaded surface candidate was found nearby."); return true; }
        std::vector<std::shared_ptr<baritone::Goal>> goals;
        for (const auto& candidate : candidates) goals.push_back(std::make_shared<baritone::GoalBlock>(candidate));
        beginManualGoal(std::make_shared<baritone::GoalComposite>(std::move(goals)));
        return true;
    }

    if (command == "y" && args.size() == 2) {
        int y{}; if (!parseInt(args[1], y)) { reply("Usage: .y level"); return true; }
        beginManualGoal(std::make_shared<baritone::GoalYLevel>(y)); return true;
    }
    if (command == "xz" && args.size() == 3) {
        int x{}, z{}; if (!parseInt(args[1], x) || !parseInt(args[2], z)) { reply("Usage: .xz x z"); return true; }
        beginManualGoal(std::make_shared<baritone::GoalXZ>(x, z)); return true;
    }
    if (command == "near" && args.size() == 5) {
        int x{}, y{}, z{}, radius{};
        if (!parseInt(args[1], x) || !parseInt(args[2], y) || !parseInt(args[3], z) || !parseInt(args[4], radius) || radius < 0) {
            reply("Usage: .near x y z radius"); return true;
        }
        beginManualGoal(std::make_shared<baritone::GoalNear>(baritone::BlockPos{x, y, z}, radius)); return true;
    }
    if ((command == "interact" || command == "twoblocks") && args.size() == 4) {
        int x{}, y{}, z{};
        if (!parseInt(args[1], x) || !parseInt(args[2], y) || !parseInt(args[3], z)) { reply("Usage: ." + command + " x y z"); return true; }
        if (command == "interact") beginManualGoal(std::make_shared<baritone::GoalGetToBlock>(baritone::BlockPos{x, y, z}));
        else beginManualGoal(std::make_shared<baritone::GoalTwoBlocks>(baritone::BlockPos{x, y, z}));
        return true;
    }
    if ((command == "goto" || command == "goal") && args.size() == 4) {
        int x{}, y{}, z{};
        if (!parseInt(args[1], x) || !parseInt(args[2], y) || !parseInt(args[3], z)) { reply("Usage: ." + command + " x y z"); return true; }
        auto goal = std::make_shared<baritone::GoalBlock>(baritone::BlockPos{x, y, z});
        stopProcesses(); enable();
        if (command == "goal") { controller.setGoal(std::move(goal)); reply("Goal set. Use .path to begin."); }
        else if (!controller.goTo(std::move(goal))) reply("Join a world before starting pathing.");
        else reply("Calculating path.");
        return true;
    }

    reply("Unknown or malformed command. Use .help.");
    return true;
}

const baritone::BaritoneController& BaritoneModule::getController() const { return controller; }

baritone::BaritoneController& BaritoneModule::getController() { return controller; }

void BaritoneModule::stopProcesses() {
    if (miningProcess.isActive())
        miningProcess.cancel(controller);
    if (exploreProcess.isActive())
        exploreProcess.cancel(controller);
    controller.stop();
}

void BaritoneModule::reply(const std::string& text) const {
    if (const auto gui = MC::getGuiData())
        gui->displayClientMessage("\xC2\xA7" "6[Limiter]" "\xC2\xA7" "r " + text);
}
