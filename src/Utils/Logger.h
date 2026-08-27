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
    static void log(const std::string& text, Args... args) {
        const std::string formatted = fmt::vformat(text, fmt::make_format_args(args...));
        logger->info(formatted);
    }
};
