#include "os.h"

#include "core/io/file_access_unix.h"
#include "core/io/std_logger.h"

namespace my {

void OS::Initialize() {
    FileAccess::MakeDefault<FileAccessUnix>(FileAccess::ACCESS_RESOURCE);
    FileAccess::MakeDefault<FileAccessUnix>(FileAccess::ACCESS_USERDATA);
    FileAccess::MakeDefault<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);

    AddLogger(std::make_shared<StdLogger>());
    AddLogger(std::make_shared<DebugConsoleLogger>());
}

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