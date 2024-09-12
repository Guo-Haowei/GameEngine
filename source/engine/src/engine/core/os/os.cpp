#include "os.h"

#include "core/io/file_access_unix.h"
#include "core/io/std_logger.h"

namespace my {

void OS::initialize() {
    FileAccess::makeDefault<FileAccessUnix>(FileAccess::ACCESS_RESOURCE);
    FileAccess::makeDefault<FileAccessUnix>(FileAccess::ACCESS_USERDATA);
    FileAccess::makeDefault<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);

    add_logger(std::make_shared<my::StdLogger>());
    // @TODO: add output debug string
}

void OS::finalize() {
}

void OS::add_logger(std::shared_ptr<ILogger> logger) {
    m_logger.addLogger(logger);
}

void OS::print(LogLevel level, std::string_view message) {
    m_logger.print(level, message);
    if (level == LOG_LEVEL_FATAL) {
        GENERATE_TRAP();
    }
}

}  // namespace my