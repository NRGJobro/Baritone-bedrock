#include "ClientHooks.h"

#include "../../../Client.h"
#include "../../../Client/GUI/ClickGui.h"
#include "../../../Client/Module/ModuleManager.h"
#include "../../../Client/Module/Modules/FullBrightModule.h"
#include "../../../Baritone/Bedrock/BedrockBlockBreaking.h"
#include "../../../SDK/Client/Input/MouseDevice.h"
#include "../../../SDK/MC.h"
#include "../../../SDK/Network/LoopbackPacketSender.h"
#include "../../../SDK/Network/Packet/Packet.h"
#include "../../../SDK/Network/Packet/Packets/PlayerAuthInputPacket.h"
#include "../../../SDK/World/Level/HitResult/FacingID.h"
#include "../../../Utils/Logger.h"
#include "../HookManager.h"

namespace {

// Diagnostic views of the same-version packet layouts. Only block-action
// fields are inspected; item, chat, login and authentication data are ignored.
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
        if (id == MinecraftPacketIds::PlayerAuthInputPacket)
            baritone::bedrock_block_breaking::rewritePlayerAuthInput(
                *static_cast<PlayerAuthInputPacket*>(packet));
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
        if (g_Client.clickGuiOpened)
            MC::getMinecraftGame()->releaseMouse();
        else
            MC::getMinecraftGame()->grabMouse();
        g_Client.blockedKeys[VK_TAB] = true;
        return;
    }

    bool cancel = false;
    ClickGui::onKey(keyCode, down, cancel);
    g_Client.blockedKeys[keyCode] = cancel;
}

LRESULT MainWindow__windowProcCallback(HWND window, const UINT message, const WPARAM wParam, const LPARAM lParam) {
    static auto original = GET_HOOK(&MainWindow__windowProcCallback);

    if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
        Keyboard_feed(wParam & 0xFF, true);
        if (g_Client.blockedKeys[wParam & 0xFF])
            return DefWindowProcW(window, message, wParam, lParam);
    } else if (message == WM_KEYUP || message == WM_SYSKEYUP) {
        Keyboard_feed(wParam & 0xFF, false);
        g_Client.blockedKeys[wParam & 0xFF] = false;
    }

    return original(window, message, wParam, lParam);
}

bool GameCore_handleMouseInput(void* first, void* second, void* third) {
    static auto original = GET_HOOK(&GameCore_handleMouseInput);
    const bool result = original(first, second, third);
    const auto mouse = MouseDevice::get();

    for (std::size_t i = 0; i < mouse->inputs.size();) {
        const auto action = mouse->inputs[i];
        bool cancel = false;

        if (action.action == 4)
            ClickGui::onWheel(action.data > 0, cancel);
        else if (action.action != 0)
            ClickGui::onMouse(action.action, action.data != 0, cancel);

        if (cancel)
            mouse->inputs.erase(std::next(mouse->inputs.begin(), i));
        else
            ++i;
    }

    return result;
}

void MinecraftGame_onDeviceLost(MinecraftGame* game) {
    static auto original = GET_HOOK(&MinecraftGame_onDeviceLost);
    if (!g_Client.clickGuiOpened)
        original(game);
}

void MinecraftGame_grabMouse(MinecraftGame* game) {
    static auto original = GET_HOOK(&MinecraftGame_grabMouse);
    if (!g_Client.clickGuiOpened)
        original(game);
}

void ClientInstanceScreenModel_sendChatMessage(void* screenModel, const std::string& message) {
    static auto original = GET_HOOK(&ClientInstanceScreenModel_sendChatMessage);
    if (!message.empty() && g_modMgr.handleChat(message))
        return;
    original(screenModel, message);
}

void MultiPlayerLevel__subTick(Level* level) {
    static auto original = GET_HOOK(&MultiPlayerLevel__subTick);
    // Feed movement into Bedrock before its normal physics and networking run.
    g_modMgr.onTick();
    original(level);
    // Limiter keeps movement/server yaw separate from displayed yaw. Apply the
    // eased visual endpoint only after Bedrock has consumed path input.
    g_modMgr.onPostTick();
}

float BaseOptions_getGamma(void* options) {
    static auto original = GET_HOOK(&BaseOptions_getGamma);
    const auto fullBright = g_modMgr.getModule<FullBrightModule>();
    if (fullBright != nullptr && fullBright->isEnabled())
        return fullBright->getIntensity();
    return original(options);
}

} // namespace

void ClientHooks::init() {
    ADD_HOOK("MainWindow::_windowProcCallback", MainWindow__windowProcCallback);
    ADD_HOOK("GameCore::handleMouseInput", GameCore_handleMouseInput);
    ADD_HOOK("MinecraftGame::onDeviceLost", MinecraftGame_onDeviceLost);
    ADD_HOOK("ClientInstanceScreenModel::sendChatMessage", ClientInstanceScreenModel_sendChatMessage);
    ADD_HOOK("MultiPlayerLevel::_subTick", MultiPlayerLevel__subTick);

    if (auto* packetSender = MC::getClientInstance()->getPacketSender(); packetSender != nullptr) {
        const auto packetSenderVtable = *reinterpret_cast<uintptr_t**>(packetSender);
        if (packetSenderVtable != nullptr)
            ADD_HOOK2(LoopbackPacketSender_sendToServer, packetSenderVtable[4]);
    }

    const auto gameVtable = *reinterpret_cast<uintptr_t**>(MC::getMinecraftGame());
    ADD_HOOK2(MinecraftGame_grabMouse, gameVtable[144]);

    // Same-version Borion uses BaseOptions vtable slot 131 for gamma. Keeping
    // this alongside the player-vision render hook covers both light-texture
    // generation paths used by Bedrock dimensions and graphics modes.
    if (auto* options = MC::getClientInstance()->getOptions()) {
        const auto optionsVtable = *reinterpret_cast<uintptr_t**>(options);
        ADD_HOOK2(BaseOptions_getGamma, optionsVtable[131]);
    }
}
