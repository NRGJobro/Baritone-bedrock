#pragma once

struct GuiMessage {
    enum class MessageType : int {
        None,
        ChatMessage,
        ClientMessage,
        LocalizedMessage,
        SystemMessage,
        WhisperMessage,
        TextObjectMessage,
        TextObjectWhisperMessage,
        AnnouncementMessage
    };

    MessageType type;
    std::string message;
    std::optional<std::string> filteredMessage;
    std::string ttsMessage;
    std::string username;
    std::string fullString;
    std::optional<std::string> filteredString;
    std::string xuid;
    bool forceVisible;
    bool ttsRequired;
    float duration;
    bool hasBeenSeen;
};
