#pragma once
#include "engine/core/io/logger.h"

namespace my {

class Win32Logger : public ILogger {
public:
    void Print(LogLevel p_level, std::string_view p_message) override;

private:
    std::mutex m_consoleMutex;
};

}  // namespace my
