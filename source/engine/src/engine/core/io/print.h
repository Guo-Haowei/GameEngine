#pragma once
#include "core/math/math.h"

#define LOG_VERBOSE(FMT, ...)   ::my::LogImpl(::my::LOG_LEVEL_VERBOSE, FMT, ##__VA_ARGS__)
#define LOG(FMT, ...)           ::my::LogImpl(::my::LOG_LEVEL_NORMAL, FMT, ##__VA_ARGS__)
#define LOG_OK(FMT, ...)        ::my::LogImpl(::my::LOG_LEVEL_OK, FMT, ##__VA_ARGS__)
#define LOG_WARN(FMT, ...)      ::my::LogImpl(::my::LOG_LEVEL_WARN, FMT, ##__VA_ARGS__)
#define LOG_ERROR(FMT, ...)     ::my::LogImpl(::my::LOG_LEVEL_ERROR, FMT, ##__VA_ARGS__)
#define LOG_FATAL(FMT, ...)     ::my::LogImpl(::my::LOG_LEVEL_FATAL, FMT, ##__VA_ARGS__)
#define PRINT_VERBOSE(FMT, ...) ::my::PrintImpl(::my::LOG_LEVEL_VERBOSE, FMT, ##__VA_ARGS__)
#define PRINT(FMT, ...)         ::my::PrintImpl(::my::LOG_LEVEL_NORMAL, FMT, ##__VA_ARGS__)
#define PRINT_OK(FMT, ...)      ::my::PrintImpl(::my::LOG_LEVEL_OK, FMT, ##__VA_ARGS__)
#define PRINT_WARN(FMT, ...)    ::my::PrintImpl(::my::LOG_LEVEL_WARN, FMT, ##__VA_ARGS__)
#define PRINT_ERROR(FMT, ...)   ::my::PrintImpl(::my::LOG_LEVEL_ERROR, FMT, ##__VA_ARGS__)
#define PRINT_FATAL(FMT, ...)   ::my::PrintImpl(::my::LOG_LEVEL_FATAL, FMT, ##__VA_ARGS__)

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

void PrintImpl(LogLevel p_level, const std::string& p_message);

template<typename... Args>
inline void PrintImpl(LogLevel p_level, std::format_string<Args...> p_format, Args&&... p_args) {
    std::string message = std::format(p_format, std::forward<Args>(p_args)...);
    PrintImpl(p_level, message);
}

void LogImpl(LogLevel p_level, const std::string& p_message);

template<typename... Args>
inline void LogImpl(LogLevel p_level, std::format_string<Args...> p_format, Args&&... p_args) {
    std::string message = std::format(p_format, std::forward<Args>(p_args)...);
    LogImpl(p_level, message);
}

}  // namespace my
