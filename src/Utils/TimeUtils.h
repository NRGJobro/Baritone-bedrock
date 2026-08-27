#pragma once

typedef long long millis;

class TimeUtils {
public:
	static millis currentTimeMillis();
    static bool hasEnoughTimePassed(millis old, millis expectedPassed);
};
