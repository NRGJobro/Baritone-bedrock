#include "ClientHooks.h"

#include "../../../Baritone/Bedrock/BedrockBlockBreaking.h"
#include "../../../Client.h"
#include "../../../Client/GUI/ClickGui.h"
#include "../../../Client/Module/ModuleManager.h"
#include "../../../Client/Module/Modules/FullBrightModule.h"
#include "../../../Baritone/Bedrock/BedrockBlockBreaking.h"
#include "../../../Baritone/Bedrock/ElytraProcess.h"
#include "../../../SDK/MC.h"
#include "../../../SDK/Network/LoopbackPacketSender.h"
#include "../../../SDK/Network/Packet/Packet.h"
#include "../../../SDK/Network/Packet/Packets/PlayerAuthInputPacket.h"
#include "../../../SDK/World/Level/HitResult/FacingID.h"
#include "../../../Utils/Logger.h"
#include "../HookManager.h"

namespace {

enum class TextPacketType : uint8_t {
    Raw, Chat, Translate, Popup, JukeboxPopup, Tip, SystemMessage, Whisper,
    Announcement, TextObjectWhisper, TextObject, TextObjectAnnouncement
};

struct TextPacketPayload {
    struct AuthorAndMessage { TextPacketType type; std::string author; std::string message; };
    struct MessageAndParams { TextPacketType type; std::string message; std::vector<std::string> params; };
    struct MessageOnly { TextPacketType type; std::string message; };

    bool localize;
    std::string xuid;
    std::string platformId;
    std::optional<std::string> filteredMessage;
    std::variant<MessageOnly, AuthorAndMessage, MessageAndParams> body;
};

// 1.26.52 TextPacket is Packet followed by the payload through non-virtual
// multiple inheritance. This view deliberately omits the trailing serialization
// mode because Limiter only reads the outgoing message.
struct TextPacketView : Packet, TextPacketPayload {};

const std::string& getTextMessage(const TextPacketView& packet) {
    return std::visit([](const auto& body) -> const std::string& { return body.message; }, packet.body);
}

struct LoggedPlayerActionPacket : Packet {
    glm::ivec3 blockPos;
    glm::ivec3 resultPos;
    int face;
    PlayerActionType action;
    std::uint64_t entityRuntimeID;
    bool isFromServerPlayerMovementSystem;
};

const char* blockActionName(const PlayerActionType action) {
    switch (action) {
    case PlayerActionType::StartDestroyBlock: return "start";
    case PlayerActionType::AbortDestroyBlock: return "abort";
    case PlayerActionType::StopDestroyBlock: return "stop";
    case PlayerActionType::CrackBlock: return "crack";
    default: return "other";
    }
}

void LoopbackPacketSender_sendToServer(LoopbackPacketSender* sender, Packet* packet) {
    static auto original = GET_HOOK(&LoopbackPacketSender_sendToServer);
    if (packet != nullptr) {
        const auto id = packet->getID();
        if (id == MinecraftPacketIds::Text) {
            const auto* textPacket = static_cast<const TextPacketView*>(packet);
            if (g_modMgr.handleChat(getTextMessage(*textPacket)))
                return;
        }
        if (id == MinecraftPacketIds::PlayerAuthInputPacket) {
            baritone::bedrock_block_breaking::rewritePlayerAuthInput(
                *static_cast<PlayerAuthInputPacket*>(packet));
            baritone::ElytraProcess::rewriteAuthInput(
                *static_cast<PlayerAuthInputPacket*>(packet));
        }
        if (id == MinecraftPacketIds::PlayerAction) {
            const auto* action = static_cast<const LoggedPlayerActionPacket*>(packet);
            const int actionValue = static_cast<int>(action->action);
            if (actionValue == 0 || actionValue == 1 || actionValue == 2 || actionValue == 18) {
                logF("[BlockPacket] PlayerAction action={}({}) pos=({}, {}, {}) face={}",
                    blockActionName(action->action), actionValue, action->blockPos.x,
                    action->blockPos.y, action->blockPos.z, action->face);
            }
        } else if (id == MinecraftPacketIds::PlayerAuthInputPacket) {
            const auto* input = static_cast<const PlayerAuthInputPacket*>(packet);
            // PerformBlockActions is input flag 35. A sane action count guard
            // prevents a stale layout from ever being dereferenced.
            const auto count = input->blockActions.size();
            if ((input->inputFlags.test(35) || count > 0) && count <= 32) {
                logF("[BlockPacket] AuthInput tick={} pitch={:.2f} yaw={:.2f} bodyYaw={:.2f} perform={} actions={} flags=0x{:X}",
                    input->clientTick, input->pitch, input->yaw, input->bodyYaw,
                    input->inputFlags.test(35), count, input->inputFlags.to_ullong());
                logF("[BlockPacket]   interact=({:.2f}, {:.2f}) camera=({:.3f}, {:.3f}, {:.3f}) model={}",
                    input->interactRotation.x, input->interactRotation.y,
                    input->cameraOrientation.x, input->cameraOrientation.y,
                    input->cameraOrientation.z, input->interactionModel);
                for (const auto& action : input->blockActions) {
                    logF("[BlockPacket]   action={}({}) pos=({}, {}, {}) face={}",
                        blockActionName(action.type), static_cast<int>(action.type),
                        action.pos.x, action.pos.y, action.pos.z, static_cast<int>(action.face));
                }
            }
        } else if (id == MinecraftPacketIds::InventoryTransaction) {
            // A survival block is not committed by CrackBlock alone. This is
            // the definitive completion packet and is intentionally logged
            // without inspecting any inventory/item payload.
            logF("[BlockPacket] InventoryTransaction sent");
        }
    }
    original(sender, packet);
}

void MinecraftGame_grabMouse(void* game) {
    static auto original = GET_HOOK(&MinecraftGame_grabMouse);
    // Minecraft calls this routine again during normal world updates. Phase
    // keeps its screen-space ClickGUI interactive by suppressing those
    // recapture attempts until the GUI closes.
    if (g_Client.clickGuiOpened)
        return;
    if (original != nullptr)
        original(game);
}

void Actor_baseTick(Actor* actor) {
    static auto original = GET_HOOK(&Actor_baseTick);
    const bool localPlayerTick = actor != nullptr && actor == MC::getLocalPlayer();

    // Phase's Pathfinder prepares its command during ActorBaseTickEvent before
    // the original ActorBaseTick. This is late enough that Minecraft has
    // populated MoveInputComponent, but early enough for vanilla acceleration,
    // collision, and packet prediction to consume the synthetic W/A/S/D state.
    if (localPlayerTick)
        g_modMgr.onTick();

    if (original != nullptr)
        original(actor);

    if (localPlayerTick)
        g_modMgr.onPostTick();
}

void ensureActorBaseTickHook(LocalPlayer* player) {
    static std::atomic_bool installed{false};
    if (player == nullptr || installed.load(std::memory_order_acquire))
        return;

    const auto vtable = *reinterpret_cast<uintptr_t**>(player);
    if (vtable == nullptr || vtable[25] == 0)
        return;

    HookManager::addHook(vtable[25], &Actor_baseTick);
    installed.store(true, std::memory_order_release);
    logF("Installed ActorBaseTick movement hook at {:#x}", vtable[25]);
}

void Keyboard_feed(const uint8_t keyCode, const bool down) {
    if (g_Client.keys[keyCode] == down)
        return;

    g_Client.keys[keyCode] = down;
    if (keyCode == VK_END && down)
        g_Client.running = false;

    if (keyCode == VK_TAB && down && MC::getLocalPlayer() != nullptr) {
        ClickGui::setOpen(!g_Client.clickGuiOpened);
        g_Client.blockedKeys[VK_TAB] = true;
        return;
    }

    bool cancel = false;
    ClickGui::onKey(keyCode, down, cancel);
    g_Client.blockedKeys[keyCode] = cancel;
}

bool feedMouseMessage(const UINT message, const WPARAM wParam) {
    int button = 0;
    bool pressed = false;
    switch (message) {
    case WM_LBUTTONDOWN: button = 1; pressed = true; break;
    case WM_LBUTTONUP: button = 1; break;
    case WM_RBUTTONDOWN: button = 2; pressed = true; break;
    case WM_RBUTTONUP: button = 2; break;
    case WM_MBUTTONDOWN: button = 3; pressed = true; break;
    case WM_MBUTTONUP: button = 3; break;
    case WM_MOUSEWHEEL: {
        bool cancel = false;
        ClickGui::onWheel(GET_WHEEL_DELTA_WPARAM(wParam) > 0, cancel);
        return cancel;
    }
    default: return false;
    }

    bool cancel = false;
    ClickGui::onMouse(button, pressed, cancel);
    return cancel;
}

LRESULT MainWindow__windowProcCallback(HWND window, const UINT message, const WPARAM wParam, const LPARAM lParam) {
    static auto original = GET_HOOK(&MainWindow__windowProcCallback);
    if (original == nullptr)
        return DefWindowProcW(window, message, wParam, lParam);

    if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
        Keyboard_feed(static_cast<uint8_t>(wParam & 0xFF), true);
        if (g_Client.blockedKeys[wParam & 0xFF])
            return 0;
    } else if (message == WM_KEYUP || message == WM_SYSKEYUP) {
        Keyboard_feed(static_cast<uint8_t>(wParam & 0xFF), false);
        if (g_Client.blockedKeys[wParam & 0xFF]) {
            g_Client.blockedKeys[wParam & 0xFF] = false;
            return 0;
        }
    } else if (feedMouseMessage(message, wParam)) {
        return 0;
    }

    return original(window, message, wParam, lParam);
}

bool ClientInstance_update(ClientInstance* instance, const uint32_t updateArgument) {
    static auto original = GET_HOOK(&ClientInstance_update);
    if (original == nullptr)
        return false;

    // The local actor does not exist on menus. Install its stable vtable hook
    // as soon as a world is joined; HookManager enables late hooks immediately.
    if (instance != nullptr)
        ensureActorBaseTickHook(instance->getLocalPlayer());
    return original(instance, updateArgument);
}

float BaseOptions_getGamma(void** options) {
    static auto original = GET_HOOK(&BaseOptions_getGamma);
    const auto fullBright = g_modMgr.getModule<FullBrightModule>();
    if (fullBright != nullptr && fullBright->isEnabled())
        return fullBright->getIntensity();
    return original == nullptr ? 1.f : original(options);
}

} // namespace

void ClientHooks::init() {
    ADD_HOOK("WindowProcCallbackHook::keymapSig", MainWindow__windowProcCallback);
    ADD_HOOK("GrabMouseHook::grabMouseSig", MinecraftGame_grabMouse);
    ADD_HOOK("UpdateHook::updateSig", ClientInstance_update);
    ADD_HOOK("GammaHook::gammaSig", BaseOptions_getGamma);

    ensureActorBaseTickHook(MC::getLocalPlayer());

    if (auto* instance = MC::getClientInstance(); instance != nullptr) {
        if (auto* packetSender = instance->getPacketSender(); packetSender != nullptr) {
            const auto packetSenderVtable = *reinterpret_cast<uintptr_t**>(packetSender);
            if (packetSenderVtable != nullptr)
                ADD_HOOK2(LoopbackPacketSender_sendToServer, packetSenderVtable[4]);
        }
    }
}
