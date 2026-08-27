#include "GuiData.h"

#include "../../../Memory/Sig/SignatureManager.h"

void GuiData::displayClientMessage(const std::string& message) {
    if (message.empty() || message.size() > 0x1000)
        return;

    using displayClientMessage = void(__fastcall*)(GuiData*, const std::string&, std::optional<std::string>, bool);
    static auto displayMessageFunc = reinterpret_cast<displayClientMessage>(GET_SIG("GuiData::displayClientMessage"));

    if (displayMessageFunc != nullptr)
        displayMessageFunc(this, message, {}, false);
}

bool GuiData::isHudElementVisible(const HudElement element) const {
    const int i = magic_enum::enum_integer(element);
    return this->hudElementVisibility[i] == 1;
}
