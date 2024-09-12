#pragma once
#include "core/base/ring_buffer.h"
#include "core/base/singleton.h"
#include "core/io/print.h"

namespace my {

class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void print(LogLevel p_level, std::string_view p_message) = 0;
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

    void print(LogLevel p_level, std::string_view p_message) override;

    void addLogger(std::shared_ptr<ILogger> p_logger);
    void addChannel(LogLevel p_log) { m_channels |= p_log; }
    void removeChannel(LogLevel p_log) { m_channels &= ~p_log; }

    // @TODO: change to array
    void retrieveLog(std::vector<Log>& p_buffer);

private:
    std::vector<std::shared_ptr<ILogger>> m_loggers;

    RingBuffer<Log, MAX_LOGS_KEPT> m_log_history;
    std::mutex m_log_history_mutex;

    int m_channels = LOG_LEVEL_ALL;
};

}  // namespace my
