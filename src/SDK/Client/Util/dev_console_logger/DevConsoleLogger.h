#pragma once

#include "../../../Core/File/OutputFileStream.h"
#include "../../../Util/TimerFacade.h"

class DevConsoleLogger : public Bedrock::EnableNonOwnerReferences {
public:
    std::string logFolder;
    Core::OutputFileStream logFile;
    std::stringstream fileBuffer;

private:
    char pad[0x8];

public:
    std::string timeStamp;
    std::function<std::string __cdecl()> getTestrunIDCallback;
    std::function<bool __cdecl()> isAutomationRunCallback;
    TimerFacade timerFacade;
};
