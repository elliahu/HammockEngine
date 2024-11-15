#pragma once
#include <iostream>
#include <cstdarg>

namespace Hmck {
    enum LogLevel {
        HMCK_LOG_LEVEL_DEBUG,
        HMCK_LOG_LEVEL_WARN,
        HMCK_LOG_LEVEL_ERROR
    };

    class Logger {
    public:
        static inline LogLevel hmckMinLogLevel = LogLevel::HMCK_LOG_LEVEL_DEBUG;

        // Create a method that works as a printf function and writes to a console log message if the level of message is same or higher as the hmckMinLogLevel;
        static inline void log(const LogLevel level, const char *format, ...) {
            if (level >= hmckMinLogLevel) {
                va_list args;
                va_start(args, format);
                vprintf(format, args);
                va_end(args);
            }
        }
    };
}
