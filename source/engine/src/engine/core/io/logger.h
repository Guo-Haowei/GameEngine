#pragma once
#include "core/base/ring_buffer.h"
#include "core/base/singleton.h"
#include "core/io/print.h"

namespace my {
// WORD is flags of FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
// clang-format off
//                  Level,              TAG         Ansi            DWORD
#define LOG_LEVEL_COLOR_LIST                                             \
    LOG_LEVEL_COLOR(LOG_LEVEL_VERBOSE,  "",         "\033[90m",     0x8) \
    LOG_LEVEL_COLOR(LOG_LEVEL_NORMAL,   "",         "\033[0m",      0x7) \
    LOG_LEVEL_COLOR(LOG_LEVEL_OK,       "[OK]",     "\033[92m",     0xA) \
    LOG_LEVEL_COLOR(LOG_LEVEL_WARN,     "[WARN] ",  "\033[93m",     0xE) \
    LOG_LEVEL_COLOR(LOG_LEVEL_ERROR,    "[ERROR]",  "\033[91m",     0xC) \
    LOG_LEVEL_COLOR(LOG_LEVEL_FATAL,    "[FATAL]",  "\033[101;30m", 0xC)
// clang-format on

class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void Print(LogLevel p_level, std::string_view p_message) = 0;
};

class StdLogger : public ILogger {
public:
    virtual void Print(LogLevel p_level, std::string_view p_message) override;
};

class CompositeLogger : public ILogger, public Singleton<CompositeLogger> {
public:
    enum {
        MAX_LOGS_KEPT = 128,
        PER_LOG_STRUCT_SIZE = 256,
    };

    struct Log {
        LogLevel level;
        char buffer[PER_LOG_STRUCT_SIZE - sizeof(level)];
    };

    void Print(LogLevel p_level, std::string_view p_message) override;

    void AddLogger(std::shared_ptr<ILogger> p_logger);
    void AddChannel(LogLevel p_log) { m_channels |= p_log; }
    void RemoveChannel(LogLevel p_log) { m_channels &= ~p_log; }

    void ClearLog();
    // @TODO: change to array
    void RetrieveLog(std::vector<Log>& p_buffer);

private:
    std::vector<std::shared_ptr<ILogger>> m_loggers;

    RingBuffer<Log, MAX_LOGS_KEPT> m_logHistory;
    std::mutex m_logHistoryMutex;

    int m_channels = LOG_LEVEL_ALL;
};

}  // namespace my
