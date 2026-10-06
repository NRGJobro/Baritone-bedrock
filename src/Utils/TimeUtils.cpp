#include "TimeUtils.h"

millis TimeUtils::currentTimeMillis() {
	const auto  time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	return time.count();
}
