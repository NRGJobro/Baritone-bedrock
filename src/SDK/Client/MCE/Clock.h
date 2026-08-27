#pragma once

namespace mce {
    struct Clock {
        float accumulatedTime;
        float lastDeltaTime;
        float lastDeltaTimeSquared;
        float currentTime;
        float timeScale;
        bool mPaused;
    };
}
