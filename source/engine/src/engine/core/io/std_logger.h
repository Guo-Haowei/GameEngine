#pragma once
#include "core/io/logger.h"

namespace my {

class StdLogger : public ILogger {
public:
    void print(LogLevel p_level, std::string_view p_message) override;

private:
    std::mutex m_console_mutex;
};

}  // namespace my
