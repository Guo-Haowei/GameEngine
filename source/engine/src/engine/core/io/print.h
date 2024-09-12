#pragma once
#include "core/math/math.h"

#define LOG_VERBOSE(FMT, ...)   ::my::logImpl(::my::LOG_LEVEL_VERBOSE, FMT, ##__VA_ARGS__)
#define LOG(FMT, ...)           ::my::logImpl(::my::LOG_LEVEL_NORMAL, FMT, ##__VA_ARGS__)
#define LOG_OK(FMT, ...)        ::my::logImpl(::my::LOG_LEVEL_OK, FMT, ##__VA_ARGS__)
#define LOG_WARN(FMT, ...)      ::my::logImpl(::my::LOG_LEVEL_WARN, FMT, ##__VA_ARGS__)
#define LOG_ERROR(FMT, ...)     ::my::logImpl(::my::LOG_LEVEL_ERROR, FMT, ##__VA_ARGS__)
#define LOG_FATAL(FMT, ...)     ::my::logImpl(::my::LOG_LEVEL_FATAL, FMT, ##__VA_ARGS__)
#define PRINT_VERBOSE(FMT, ...) ::my::printImpl(::my::LOG_LEVEL_VERBOSE, FMT, ##__VA_ARGS__)
#define PRINT(FMT, ...)         ::my::printImpl(::my::LOG_LEVEL_NORMAL, FMT, ##__VA_ARGS__)
#define PRINT_OK(FMT, ...)      ::my::printImpl(::my::LOG_LEVEL_OK, FMT, ##__VA_ARGS__)
#define PRINT_WARN(FMT, ...)    ::my::printImpl(::my::LOG_LEVEL_WARN, FMT, ##__VA_ARGS__)
#define PRINT_ERROR(FMT, ...)   ::my::printImpl(::my::LOG_LEVEL_ERROR, FMT, ##__VA_ARGS__)
#define PRINT_FATAL(FMT, ...)   ::my::printImpl(::my::LOG_LEVEL_FATAL, FMT, ##__VA_ARGS__)

namespace my {

enum LogLevel : uint8_t {
    LOG_LEVEL_VERBOSE = BIT(1),
    LOG_LEVEL_NORMAL = BIT(2),
    LOG_LEVEL_OK = BIT(3),
    LOG_LEVEL_WARN = BIT(4),
    LOG_LEVEL_ERROR = BIT(5),
    LOG_LEVEL_FATAL = BIT(6),
    LOG_LEVEL_ALL = LOG_LEVEL_VERBOSE |
                    LOG_LEVEL_NORMAL |
                    LOG_LEVEL_OK |
                    LOG_LEVEL_WARN |
                    LOG_LEVEL_ERROR |
                    LOG_LEVEL_FATAL,
};

void printImpl(LogLevel p_level, const std::string& p_message);

template<typename... Args>
inline void printImpl(LogLevel p_level, std::format_string<Args...> p_format, Args&&... p_args) {
    std::string message = std::format(p_format, std::forward<Args>(p_args)...);
    printImpl(p_level, message);
}

void logImpl(LogLevel p_level, const std::string& p_message);

template<typename... Args>
inline void logImpl(LogLevel p_level, std::format_string<Args...> p_format, Args&&... p_args) {
    std::string message = std::format(p_format, std::forward<Args>(p_args)...);
    logImpl(p_level, message);
}

}  // namespace my
