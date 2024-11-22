#include "os.h"

#include "core/io/file_access_unix.h"

namespace my {

void OS::Finalize() {
}

void OS::AddLogger(std::shared_ptr<ILogger> p_logger) {
    m_logger.AddLogger(p_logger);
}

void OS::Print(LogLevel level, std::string_view p_message) {
    m_logger.Print(level, p_message);
    if (level & LOG_LEVEL_FATAL) {
        GENERATE_TRAP();
    }
}

}  // namespace my