#include "ClientHooks.h"

#include "../../../Client.h"
#include "../../../Client/GUI/ClickGui.h"
#include "../../../Client/Module/ModuleManager.h"
#include "../../../SDK/Client/Input/MouseDevice.h"
#include "../../../SDK/MC.h"
#include "../HookManager.h"

namespace {

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

} // namespace

void ClientHooks::init() {
    ADD_HOOK("MainWindow::_windowProcCallback", MainWindow__windowProcCallback);
    ADD_HOOK("GameCore::handleMouseInput", GameCore_handleMouseInput);
    ADD_HOOK("MinecraftGame::onDeviceLost", MinecraftGame_onDeviceLost);
    ADD_HOOK("ClientInstanceScreenModel::sendChatMessage", ClientInstanceScreenModel_sendChatMessage);
    ADD_HOOK("MultiPlayerLevel::_subTick", MultiPlayerLevel__subTick);

    const auto gameVtable = *reinterpret_cast<uintptr_t**>(MC::getMinecraftGame());
    ADD_HOOK2(MinecraftGame_grabMouse, gameVtable[144]);
}
