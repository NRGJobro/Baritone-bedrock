#pragma once

struct TitleMessage {
    std::string title;
    std::optional<std::string> filteredTitle;
    std::string subtitle;
    std::optional<std::string> filteredSubtitle;
    int fadeInTime;
    int stayTime;
    int fadeOutTime;
    std::string actionBarMessage;
    std::optional<std::string> filteredActionBarMessage;
};
