#include "logger.h"

#include "core/base/ring_buffer.h"

namespace my {

void CompositeLogger::addLogger(std::shared_ptr<ILogger> p_logger) {
    m_loggers.emplace_back(p_logger);
}

void CompositeLogger::print(LogLevel p_level, std::string_view p_message) {
    // @TODO: set verbose
    if (!(m_channels & p_level)) {
        return;
    }

    for (auto& logger : m_loggers) {
        logger->print(p_level, p_message);
    }

    m_log_history_mutex.lock();
    m_log_history.push_back({});
    auto& log_history = m_log_history.back();
    log_history.level = p_level;
    strncpy(log_history.buffer, p_message.data(), sizeof(log_history.buffer) - 1);
    m_log_history_mutex.unlock();
}

void CompositeLogger::retrieveLog(std::vector<Log>& p_buffer) {
    m_log_history_mutex.lock();

    for (auto& log : m_log_history) {
        p_buffer.push_back(log);
    }

    m_log_history_mutex.unlock();
}

}  // namespace my
