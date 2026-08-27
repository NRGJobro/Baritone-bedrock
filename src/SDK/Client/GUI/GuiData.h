#pragma once

#include "../../Bedrock/EnableNonOwnerReferences.h"
#include "../../Bedrock/NonOwnerPointer.h"
#include "../../Core/AppPlatformListener.h"
#include "../../Core/IConfigListener.h"
#include "../../Render/RectangleArea.h"
#include "../../World/Inventory/ContainerID.h"
#include "../../World/Inventory/PlayerInventory.h"
#include "../MCE/Mesh.h"
#include "../Util/CoordinatesCaptureType.h"
#include "../Util/dev_console_logger/DevConsoleLogger.h"
#include "../ui/SliceSize.h"
#include "GuiMessage.h"
#include "HudElement.h"
#include "MenuPointer.h"
#include "ScreenSizeData.h"
#include "TitleMessage.h"

class GuiData : public IConfigListener, public AppPlatformListener, public Bedrock::EnableNonOwnerReferences {
public:
    ScreenSizeData screenSizeData;
    bool screenSizeDataValid;
    float guiScale;
    float invGuiScale;
    bool isCurrentlyActive;
    std::set<int> postedErrors;
    MenuPointer menuPointer;
    int16_t mouseX;
    int16_t mouseY;
    bool hasShowPreexistingMessages;
    bool toolbarWasRendered;
    int prevSelectedSlot;
    ContainerID prevSelectedInventoryContainer;
    int numSlots;
    int flashSlotId;
    double flashSlotStartTime;
    class ClientInstance* clientInstance;
    RectangleArea toolbarArea;
    RectangleArea toolbarAreaContainer;
    std::string lastPopupText;
    std::string lastPopupSubtitleText;
    std::string lastJukeboxPopupText;
    std::string lastJukeboxPopupSubtitleText;
    int tickCount;
    float itemNameOverlayTime;
    float jukeboxNameOverlayTime;
    bool popupNoticeDirty;
    bool jukeboxPopupNoticeDirty;
    std::vector<GuiMessage> guiMessages;
    std::vector<std::string> devConsoleMessages;
    int maxDevConsoleMessages;
    std::vector<std::string> contentLogMessages;
    int contentLogMessagesCount;
    std::vector<std::string> perfTurtleMessages;
    TitleMessage titleMessage;
    int hudElementVisibility[13];
    int serverSettingsId;
    std::string serverSettings;
    bool muteChat;
    float currentDropTicks;
    PlayerInventory::SlotData currentDropSlot;
    PlayerInventory::SlotData lastSelectedSlot;
    bool showProgress;
    std::string tipMessage;
    float tipMessageLength;
    mce::Mesh rcFeedbackInner;
    mce::Mesh rcFeedbackOuter;
    mce::Mesh vignette;
    mce::MaterialPtr invFillMat;
    mce::MaterialPtr cursorMat;
    Bedrock::NonOwnerPointerRef<DevConsoleLogger> devConsoleLogger; // NotNullNonOwnerPointerRef
    std::chrono::time_point<std::chrono::steady_clock> lastTickTime;
    std::map<std::string, std::vector<GuiMessage>> delayedMessages;
    std::vector<std::string> queuedDevConsoleMessages;
    std::mutex queuedDevMessagesMutex;
    bool useEditorGuiScale;
    ui::SliceSize hudHotbarRectangle;
    CoordinatesCaptureType coordinatesCaptureType;

    template <typename... Args>
    void displayClientMessageF(const std::string& text, Args... args) {
        const std::string formatted = fmt::vformat(text, fmt::make_format_args(args...));
        displayClientMessage(formatted);
    }

    void displayClientMessage(const std::string& message);

    void clearChat() {
        guiMessages.clear();
    }

    [[nodiscard]] bool isHudElementVisible(HudElement element) const;
};