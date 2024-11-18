#pragma once
#include "core/base/ring_buffer.h"
#include "core/base/singleton.h"
#include "core/io/print.h"

namespace my {

class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void Print(LogLevel p_level, std::string_view p_message) = 0;
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
