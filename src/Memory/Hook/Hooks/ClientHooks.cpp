#include "ClientHooks.h"

#include "../../../Baritone/Bedrock/BedrockBlockBreaking.h"
#include "../../../Client.h"
#include "../../../Client/GUI/ClickGui.h"
#include "../../../Client/Modules/CameraTweaksModule.h"
#include "../../../Client/Modules/FullBrightModule.h"
#include "../../../Client/Modules/GuiMoveModule.h"
#include "../../../Client/Modules/LimiterModule.h"
#include "../../../Client/Modules/ModuleManager.h"
#include "../../../Baritone/Bedrock/ElytraProcess.h"
#include "../../../SDK/MC.h"
#include "../../../SDK/Client/ClientInstance.h"
#include "../../../SDK/Network/LoopbackPacketSender.h"
#include "../../../SDK/Network/Packet/Packet.h"
#include "../../../SDK/Network/Packet/Packets/PlayerAuthInputPacket.h"
#include "../../../SDK/Client/Input/MoveInputComponent.h"
#include "../../../SDK/World/Actor/Actor.h"
#include "../../../SDK/World/Actor/LocalPlayer.h"
#include "../../../SDK/World/Level/HitResult/FacingID.h"
#include "../../../Utils/Logger.h"
#include "../../../Utils/Utils.h"
#include "../HookManager.h"

namespace {

struct MouseAction {
    std::int16_t x;
    std::int16_t y;
    std::int16_t dx;
    std::int16_t dy;
    std::int8_t action;
    std::int8_t data;
    int pointerId;
    bool forceMotionlessPointer;
};

struct MouseDevice {
    std::int16_t clickX;
    std::int16_t clickY;
    std::int16_t x;
    std::int16_t y;
    std::int16_t dx;
    std::int16_t dy;
    std::int16_t xOld;
    std::int16_t yOld;
    bool buttonStates[7];
    std::vector<MouseAction> inputs;
    std::int32_t firstMovementType;
};

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

// PlayerAuthInputPacket has 65 input flags. std::bitset<65>::to_ullong()
// throws when bit 64 is set, which is a valid Bedrock input flag. Packet
// diagnostics run inside the send hook, so an exception here terminates the
// game. Split the high bit from the lower word and keep logging noexcept.
std::uint64_t inputFlagsLowWord(const std::bitset<65>& flags) noexcept {
    std::uint64_t value = 0;
    for (std::size_t bit = 0; bit < 64; ++bit) {
        if (flags.test(bit))
            value |= std::uint64_t{1} << bit;
    }
    return value;
}

void handleWorldIdentityChange(ClientInstance* instance) {
    static LocalPlayer* previousPlayer = nullptr;
    static BlockSource* previousRegion = nullptr;
    static bool initialized = false;

    auto* currentPlayer = instance == nullptr ? nullptr : instance->getLocalPlayer();
    auto* currentRegion = instance == nullptr ? nullptr : instance->getBlockSource();

    if (!initialized) {
        previousPlayer = currentPlayer;
        previousRegion = currentRegion;
        initialized = true;
        return;
    }

    if (currentPlayer == previousPlayer && currentRegion == previousRegion)
        return;

    previousPlayer = currentPlayer;
    previousRegion = currentRegion;

    // Never let pathing/input/packet state survive a LocalPlayer or
    // BlockSource identity change. Dimension changes can swap either pointer
    // without unloading the DLL.
    g_Client.gameplayInputAllowed.store(false, std::memory_order_release);
    g_Client.hudScreenActive.store(false, std::memory_order_release);
    g_Client.keys.reset();
    g_Client.blockedKeys.reset();
    baritone::bedrock_block_breaking::reset();
    ClickGui::dismissForWorldChange();

    if (g_Client.modulesReady.load(std::memory_order_acquire)) {
        if (auto* limiter = g_modMgr.getModule<LimiterModule>())
            limiter->onWorldChanged();
    }
}

void LoopbackPacketSender_sendToServer(LoopbackPacketSender* sender, Packet* packet) {
    HookManager::CallbackGuard callbackGuard;
    static auto original = GET_HOOK(&LoopbackPacketSender_sendToServer);
    if (!callbackGuard.allowClientCode()) {
        if (original != nullptr)
            original(sender, packet);
        return;
    }
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
                logF("[BlockPacket] AuthInput tick={} pitch={:.2f} yaw={:.2f} bodyYaw={:.2f} perform={} actions={} flags=0x{}{:016X}",
                    input->clientTick, input->pitch, input->yaw, input->bodyYaw,
                    input->inputFlags.test(35), count,
                    input->inputFlags.test(64) ? 1 : 0,
                    inputFlagsLowWord(input->inputFlags));
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
        } else if (id == MinecraftPacketIds::PlayerEquipment) {
            logF("[BridgePacket] PlayerEquipment sent");
        } else if (id == MinecraftPacketIds::InventoryTransaction) {
            // A survival block is not committed by CrackBlock alone. This is
            // the definitive completion packet and is intentionally logged
            // without inspecting any inventory/item payload.
            logF("[BlockPacket] InventoryTransaction sent");
        }
    }
    if (original != nullptr)
        original(sender, packet);
}

void MinecraftGame_grabMouse(void* game) {
    HookManager::CallbackGuard callbackGuard;
    static auto original = GET_HOOK(&MinecraftGame_grabMouse);
    if (!callbackGuard.allowClientCode()) {
        if (original != nullptr)
            original(game);
        return;
    }
    // Minecraft calls this routine again during normal world updates. Phase
    // keeps its screen-space ClickGUI interactive by suppressing those
    // recapture attempts until the GUI closes.
    if (g_Client.clickGuiOpened)
        return;
    if (original != nullptr)
        original(game);
}

void GameControllerHandler_GameCore_refresh(void* handler) {
    HookManager::CallbackGuard callbackGuard;
    static auto original = GET_HOOK(&GameControllerHandler_GameCore_refresh);
    if (original != nullptr)
        original(handler);
    if (!callbackGuard.allowClientCode())
        return;

    const auto signature = GET_SIG("MouseDevice::instance");
    if (signature == 0)
        return;
    auto* mouse = Utils::getFromOffset<MouseDevice*>(signature, 2);
    if (mouse == nullptr)
        return;

    if (!g_Client.clickGuiOpened)
        return;

    // Phase cancels mouse input at the queue Minecraft actually consumes.
    // WndProc and raw-input cancellation happen too early to cover this path.

    std::fill(std::begin(mouse->buttonStates), std::end(mouse->buttonStates), false);
    std::erase_if(mouse->inputs, [](const MouseAction& action) {
        return action.action >= 1 && action.action <= 4;
    });
}

bool ExternalDataMultiPlayerLevel_isInWorldAndNotShowingAnyMenuScreens(void* level) {
    HookManager::CallbackGuard callbackGuard;
    static auto original = GET_HOOK(&ExternalDataMultiPlayerLevel_isInWorldAndNotShowingAnyMenuScreens);
    if (!callbackGuard.allowClientCode())
        return original != nullptr && original(level);
    if (g_Client.clickGuiOpened)
        return false;
    const bool nativeResult = original != nullptr && original(level);
    const auto guiMove = g_modMgr.getModule<GuiMoveModule>();
    return nativeResult || (guiMove != nullptr && guiMove->isEnabled() &&
        MC::getLocalPlayer() != nullptr);
}

void Actor_baseTick(Actor* actor) {
    HookManager::CallbackGuard callbackGuard;
    static auto original = GET_HOOK(&Actor_baseTick);
    if (!callbackGuard.allowClientCode()) {
        if (original != nullptr)
            original(actor);
        return;
    }
    const bool localPlayerTick = actor != nullptr && actor == MC::getLocalPlayer();
    static std::uint64_t movementTraceTick = 0;
    const bool traceMovement = localPlayerTick && (++movementTraceTick % 20 == 0);
    const auto traceInput = [traceMovement](const char* stage, Actor* currentActor) {
        if (!traceMovement || currentActor == nullptr)
            return;
        auto* input = currentActor->tryGet<MoveInputComponent>();
        const auto position = currentActor->getPosition();
        if (input == nullptr) {
            logF("[MovementTrace] {} actor={:#x} input=null pos=({:.3f},{:.3f},{:.3f})",
                stage, reinterpret_cast<std::uintptr_t>(currentActor), position.x, position.y, position.z);
            return;
        }
        logF("[MovementTrace] {} actor={:#x} input={:#x} move=({:.3f},{:.3f}) analog=({:.3f},{:.3f}) "
             "dirs={}{}{}{} locked={} cameraRel={} rotByMove={} pos=({:.3f},{:.3f},{:.3f})",
            stage, reinterpret_cast<std::uintptr_t>(currentActor), reinterpret_cast<std::uintptr_t>(input),
            input->move.x, input->move.y,
            input->rawInputState.analogMoveVector.x, input->rawInputState.analogMoveVector.y,
            input->rawInputState.up ? 'U' : '-', input->rawInputState.down ? 'D' : '-',
            input->rawInputState.left ? 'L' : '-', input->rawInputState.right ? 'R' : '-',
            input->moveInputStateLocked, input->isCameraRelativeMovementEnabled,
            input->isRotControlledByMoveDirection, position.x, position.y, position.z);
    };

    // Phase's Pathfinder prepares its command during ActorBaseTickEvent before
    // the original ActorBaseTick. This is late enough that Minecraft has
    // populated MoveInputComponent, but early enough for vanilla acceleration,
    // collision, and packet prediction to consume the synthetic W/A/S/D state.
    if (localPlayerTick)
        g_modMgr.onTick();
    traceInput("command", actor);

    if (original != nullptr)
        original(actor);
    traceInput("native", actor);

    if (localPlayerTick)
        g_modMgr.onPostTick();
    traceInput("reapply", actor);
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
        if (!cancel) {
            const auto cameraTweaks = g_modMgr.getModule<CameraTweaksModule>();
            const bool controlHeld =
                g_Client.keys[VK_CONTROL] ||
                g_Client.keys[VK_LCONTROL] ||
                g_Client.keys[VK_RCONTROL] ||
                (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            cancel = cameraTweaks != nullptr && cameraTweaks->isEnabled() &&
                cameraTweaks->onWheel(GET_WHEEL_DELTA_WPARAM(wParam) > 0, controlHeld);
        }
        return cancel;
    }
    default: return false;
    }

    bool cancel = false;
    ClickGui::onMouse(button, pressed, cancel);
    return cancel;
}

LRESULT MainWindow__windowProcCallback(HWND window, const UINT message, const WPARAM wParam, const LPARAM lParam) {
    HookManager::CallbackGuard callbackGuard;
    static auto original = GET_HOOK(&MainWindow__windowProcCallback);
    if (original == nullptr)
        return DefWindowProcW(window, message, wParam, lParam);
    if (!callbackGuard.allowClientCode())
        return original(window, message, wParam, lParam);

    // Minecraft also consumes Raw Input independently of the ordinary button
    // messages. Swallow it while ClickGUI owns the mouse, otherwise the same
    // left click toggles a module and reaches the game as an attack/swing.
    if (g_Client.clickGuiOpened && message == WM_INPUT) {
        DefWindowProcW(window, message, wParam, lParam);
        return 0;
    }

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
    HookManager::CallbackGuard callbackGuard;
    static auto original = GET_HOOK(&ClientInstance_update);
    if (original == nullptr)
        return false;

    const bool result = original(instance, updateArgument);
    MC::setClientInstance(instance);

    if (!callbackGuard.allowClientCode())
        return result;

    // Inspect identity only after Minecraft finishes the update. This avoids
    // touching a LocalPlayer/BlockSource while the native update is replacing
    // it during a disconnect or dimension transition.
    handleWorldIdentityChange(instance);
    if (instance != nullptr)
        ensureActorBaseTickHook(instance->getLocalPlayer());
    return result;
}

float BaseOptions_getGamma(void** options) {
    HookManager::CallbackGuard callbackGuard;
    static auto original = GET_HOOK(&BaseOptions_getGamma);
    if (!callbackGuard.allowClientCode())
        return original == nullptr ? 1.f : original(options);
    const auto fullBright = g_modMgr.getModule<FullBrightModule>();
    if (fullBright != nullptr && fullBright->isEnabled())
        return fullBright->getIntensity();
    return original == nullptr ? 1.f : original(options);
}

} // namespace

void ClientHooks::init() {
    ADD_HOOK("WindowProcCallbackHook::keymapSig", MainWindow__windowProcCallback);
    ADD_HOOK("GrabMouseHook::grabMouseSig", MinecraftGame_grabMouse);
    ADD_HOOK("MouseHook::mouseSig", GameControllerHandler_GameCore_refresh);
    ADD_HOOK("UpdateHook::updateSig", ClientInstance_update);
    ADD_HOOK("GammaHook::gammaSig", BaseOptions_getGamma);
    ADD_HOOK("WorldNotShowingMenusHook::worldMenusSig",
        ExternalDataMultiPlayerLevel_isInWorldAndNotShowingAnyMenuScreens);

    ensureActorBaseTickHook(MC::getLocalPlayer());

    if (auto* instance = MC::getClientInstance(); instance != nullptr) {
        if (auto* packetSender = instance->getPacketSender(); packetSender != nullptr) {
            const auto packetSenderVtable = *reinterpret_cast<uintptr_t**>(packetSender);
            if (packetSenderVtable != nullptr)
                ADD_HOOK2(LoopbackPacketSender_sendToServer, packetSenderVtable[4]);
        }
    }
}
