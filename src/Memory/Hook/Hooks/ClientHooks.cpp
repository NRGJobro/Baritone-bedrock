#include "ClientHooks.h"

#include "../../../Baritone/Bedrock/BedrockBlockBreaking.h"
#include "../../../Client.h"
#include "../../../Client/GUI/ClickGui.h"
#include "../../../Client/Module/ModuleManager.h"
#include "../../../Client/Module/Modules/FullBrightModule.h"
#include "../../../Baritone/Bedrock/BedrockBlockBreaking.h"
#include "../../../Baritone/Bedrock/ElytraProcess.h"
#include "../../../SDK/Client/Input/MouseDevice.h"
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
                if (input->inputFlags.test(34)) {
                    // PlayerAuthInput stores a unique_ptr to
                    // PackedItemUseLegacyInventoryTransaction in the first
                    // eight bytes after clientTick. Log only its scalar
                    // ItemUse transaction fields; item/NBT data is ignored.
                    const auto* packed = *reinterpret_cast<std::byte* const*>(input->padding);
                    if (packed != nullptr) {
                        const auto readInt = [packed](const std::size_t offset) {
                            int value{};
                            std::memcpy(&value, packed + offset, sizeof(value));
                            return value;
                        };
                        const auto readByte = [packed](const std::size_t offset) {
                            return std::to_integer<unsigned int>(packed[offset]);
                        };
                        const auto readVec = [packed](const std::size_t offset) {
                            glm::vec3 value{};
                            std::memcpy(&value, packed + offset, sizeof(value));
                            return value;
                        };
                        // Packed header is 40 bytes; ItemUse's base occupies
                        // 104 bytes in this build.
                        constexpr std::size_t itemUse = 40;
                        const glm::ivec3 transactionPos{readInt(itemUse + 112),
                            readInt(itemUse + 116), readInt(itemUse + 120)};
                        const auto from = readVec(itemUse + 232);
                        const auto click = readVec(itemUse + 244);
                        logF("[BlockTransaction] action={} trigger={} pos=({}, {}, {}) targetId={} face={} slot={} from=({:.3f}, {:.3f}, {:.3f}) click=({:.3f}, {:.3f}, {:.3f}) predicted={} cooldown={}",
                            readInt(itemUse + 104), readByte(itemUse + 108),
                            transactionPos.x, transactionPos.y, transactionPos.z,
                            readInt(itemUse + 124), readByte(itemUse + 128),
                            readInt(itemUse + 132), from.x, from.y, from.z,
                            click.x, click.y, click.z, readByte(itemUse + 256),
                            readByte(itemUse + 257));
                    }
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

void Keyboard_feed(const uint8_t keyCode, const bool down) {
    if (g_Client.keys[keyCode] == down)
        return;

    g_Client.keys[keyCode] = down;
    if (keyCode == VK_END && down)
        g_Client.running = false;

    if (keyCode == VK_TAB && down && MC::getLocalPlayer() != nullptr) {
        g_Client.clickGuiOpened = !g_Client.clickGuiOpened;
        if (auto* game = MC::getMinecraftGame(); game != nullptr) {
            if (g_Client.clickGuiOpened)
                game->releaseMouse();
            else
                game->grabMouse();
        }
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

    // Run movement before Bedrock consumes input, then apply the visual endpoint
    // after the native update. Menus receive the untouched native update only.
    const bool inWorld = instance != nullptr && instance->getLocalPlayer() != nullptr;
    if (inWorld)
        g_modMgr.onTick();
    const bool result = original(instance, updateArgument);
    if (inWorld)
        g_modMgr.onPostTick();
    return result;
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
    ADD_HOOK("UpdateHook::updateSig", ClientInstance_update);
    ADD_HOOK("GammaHook::gammaSig", BaseOptions_getGamma);

    if (auto* instance = MC::getClientInstance(); instance != nullptr) {
        if (auto* packetSender = instance->getPacketSender(); packetSender != nullptr) {
            const auto packetSenderVtable = *reinterpret_cast<uintptr_t**>(packetSender);
            if (packetSenderVtable != nullptr)
                ADD_HOOK2(LoopbackPacketSender_sendToServer, packetSenderVtable[4]);
        }
    }
}
