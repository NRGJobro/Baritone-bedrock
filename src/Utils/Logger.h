#pragma once

#ifndef logF
#define logF(x, ...) Logger::log(x, __VA_ARGS__)
#endif

class Logger {
    static std::shared_ptr<spdlog::logger> logger;

public:
    static void initializeLogger();

    static void clearLogs();

    template <typename... Args>
    static void log(const std::string& text, Args... args) noexcept {
        try {
            const auto activeLogger = logger;
            if (activeLogger == nullptr)
                return;
            const std::string formatted = fmt::vformat(text, fmt::make_format_args(args...));
            activeLogger->info(formatted);
        } catch (...) {
            // Diagnostics must never unwind through a Minecraft hook.
        }
    }
};
