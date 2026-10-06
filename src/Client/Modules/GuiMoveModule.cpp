#include "GuiMoveModule.h"

#include "../../Client.h"
#include "../../SDK/Client/Input/MoveInputComponent.h"
#include "../../SDK/MC.h"
#include "../../SDK/World/Actor/LocalPlayer.h"

GuiMoveModule::GuiMoveModule() : Module("Allows normal movement while inventory and supported GUI screens are open") {}

std::string GuiMoveModule::getName() {
    return "GuiMove";
}

void GuiMoveModule::onTick() {
    auto* player = MC::getLocalPlayer();
    if (player == nullptr)
        return;

    auto* input = player->tryGet<MoveInputComponent>();
    if (input == nullptr)
        return;

    input->rawInputState.up = g_Client.keys['W'];
    input->rawInputState.down = g_Client.keys['S'];
    input->rawInputState.left = g_Client.keys['A'];
    input->rawInputState.right = g_Client.keys['D'];
    input->rawInputState.jumpDown = g_Client.keys[VK_SPACE];
    input->rawInputState.sprintDown = g_Client.keys[VK_CONTROL];
}
