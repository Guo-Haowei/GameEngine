#include "os.h"

#include "core/io/file_access_unix.h"
#include "core/io/std_logger.h"

namespace my {

void OS::initialize() {
    FileAccess::makeDefault<FileAccessUnix>(FileAccess::ACCESS_RESOURCE);
    FileAccess::makeDefault<FileAccessUnix>(FileAccess::ACCESS_USERDATA);
    FileAccess::makeDefault<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);

    addLogger(std::make_shared<StdLogger>());
    addLogger(std::make_shared<DebugConsoleLogger>());
}

void OS::finalize() {
}

void OS::addLogger(std::shared_ptr<ILogger> p_logger) {
    m_logger.addLogger(p_logger);
}

void OS::print(LogLevel level, std::string_view p_message) {
    m_logger.print(level, p_message);
    if (level & LOG_LEVEL_FATAL) {
        GENERATE_TRAP();
    }
}

}  // namespace my