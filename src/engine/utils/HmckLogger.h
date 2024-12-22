#pragma once
#include <cstdarg>

namespace Hmck {
    enum LogLevel {
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERROR
    };

    class Logger {
    public:
#ifdef NDEBUG
        static inline LogLevel hmckMinLogLevel = LOG_LEVEL_ERROR;
#else
        static inline LogLevel hmckMinLogLevel = LOG_LEVEL_DEBUG;
#endif

        static void log(const LogLevel level, const char *format, ...) {
            if (level >= hmckMinLogLevel) {
                va_list args;
                va_start(args, format);
                vprintf(format, args);
                va_end(args);
            }
        }
    };
}
