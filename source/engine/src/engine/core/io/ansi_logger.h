#pragma once
#include "core/io/logger.h"

namespace my {

class AnsiLogger : public ILogger {
public:
    void Print(LogLevel p_level, std::string_view p_message) override;
};

}  // namespace my
