#pragma once

struct BasicTimer {
    double timeDelay;
    double startTime;
    std::function<double()> getCurrentTimeCallback;
};
