#include "logger.h"

#include "engine/core/base/ring_buffer.h"

namespace my {

void StdLogger::Print(LogLevel p_level, std::string_view p_message) {
    const char* tag = "";
    switch (p_level) {
#define LOG_LEVEL_COLOR(LEVEL, TAG, ANSI, WINCOLOR) \
    case LEVEL:                                     \
        tag = TAG;                                  \
        break;
        LOG_LEVEL_COLOR_LIST
#undef LOG_LEVEL_COLOR
        default:
            break;
    }

    // @TODO: stderr vs stdout
    FILE* file = stdout;
    fflush(file);

    fprintf(file, "%s%.*s", tag, static_cast<int>(p_message.length()), p_message.data());
    fflush(file);
}

void CompositeLogger::AddLogger(std::shared_ptr<ILogger> p_logger) {
    m_loggers.emplace_back(p_logger);
}

void CompositeLogger::Print(LogLevel p_level, std::string_view p_message) {
    // @TODO: set verbose
    if (!(m_channels & p_level)) {
        return;
    }

    for (auto& logger : m_loggers) {
        logger->Print(p_level, p_message);
    }

    m_logHistoryMutex.lock();
    m_logHistory.push_back({});
    auto& log_history = m_logHistory.back();
    log_history.level = p_level;
    strncpy(log_history.buffer, p_message.data(), sizeof(log_history.buffer) - 1);
    m_logHistoryMutex.unlock();
}

void CompositeLogger::ClearLog() {
    m_logHistoryMutex.lock();
    m_logHistory.clear();
    m_logHistoryMutex.unlock();
}

void CompositeLogger::RetrieveLog(std::vector<Log>& p_buffer) {
    m_logHistoryMutex.lock();

    for (auto& log : m_logHistory) {
        p_buffer.push_back(log);
    }

    m_logHistoryMutex.unlock();
}

}  // namespace my
