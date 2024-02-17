#pragma once
#include "core/math/math.h"

#define LOG_VERBOSE(FMT, ...)   ::my::log_impl(::my::LOG_LEVEL_VERBOSE, FMT, ##__VA_ARGS__)
#define LOG(FMT, ...)           ::my::log_impl(::my::LOG_LEVEL_NORMAL, FMT, ##__VA_ARGS__)
#define LOG_OK(FMT, ...)        ::my::log_impl(::my::LOG_LEVEL_OK, FMT, ##__VA_ARGS__)
#define LOG_WARN(FMT, ...)      ::my::log_impl(::my::LOG_LEVEL_WARN, FMT, ##__VA_ARGS__)
#define LOG_ERROR(FMT, ...)     ::my::log_impl(::my::LOG_LEVEL_ERROR, FMT, ##__VA_ARGS__)
#define LOG_FATAL(FMT, ...)     ::my::log_impl(::my::LOG_LEVEL_FATAL, FMT, ##__VA_ARGS__)
#define PRINT_VERBOSE(FMT, ...) ::my::print_impl(::my::LOG_LEVEL_VERBOSE, FMT, ##__VA_ARGS__)
#define PRINT(FMT, ...)         ::my::print_impl(::my::LOG_LEVEL_NORMAL, FMT, ##__VA_ARGS__)
#define PRINT_OK(FMT, ...)      ::my::print_impl(::my::LOG_LEVEL_OK, FMT, ##__VA_ARGS__)
#define PRINT_WARN(FMT, ...)    ::my::print_impl(::my::LOG_LEVEL_WARN, FMT, ##__VA_ARGS__)
#define PRINT_ERROR(FMT, ...)   ::my::print_impl(::my::LOG_LEVEL_ERROR, FMT, ##__VA_ARGS__)
#define PRINT_FATAL(FMT, ...)   ::my::print_impl(::my::LOG_LEVEL_FATAL, FMT, ##__VA_ARGS__)

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

void print_impl(LogLevel level, const std::string& message);

template<typename... Args>
inline void print_impl(LogLevel level, std::format_string<Args...> format, Args&&... args) {
    std::string message = std::format(format, std::forward<Args>(args)...);
    print_impl(level, message);
}

void log_impl(LogLevel level, const std::string& message);

template<typename... Args>
inline void log_impl(LogLevel level, std::format_string<Args...> format, Args&&... args) {
    std::string message = std::format(format, std::forward<Args>(args)...);
    log_impl(level, message);
}

}  // namespace my
