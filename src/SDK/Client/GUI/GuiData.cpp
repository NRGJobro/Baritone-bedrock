#include "GuiData.h"

#include "../../../Memory/Sig/SignatureManager.h"

void GuiData::displayClientMessage(const std::string& message) {
    if (message.empty() || message.size() > 0x1000)
        return;

    using displayClientMessage = void(__fastcall*)(GuiData*, const std::string&, void*, bool);
    static auto displayMessageFunc = reinterpret_cast<displayClientMessage>(GET_SIG("GuiData::displayClientMessage"));

    // The current game dereferences an internal message context. Phase uses
    // 0x120 bytes of aligned storage here; a null context is not valid.
    struct alignas(16) MessageContext { std::byte storage[0x120]{}; };
    static MessageContext context;
    if (displayMessageFunc != nullptr)
        displayMessageFunc(this, message, &context, false);
}
