#include "print.h"

#include "core/io/std_logger.h"
#include "core/os/os.h"
#include "core/os/threads.h"

namespace my {

void printImpl(LogLevel p_level, const std::string& p_message) {
    OS* os = OS::singleton_ptr();
    if (os) [[likely]] {
        os->print(p_level, p_message);
    } else {
        StdLogger logger;
        logger.print(p_level, p_message);
    }
}

void logImpl(LogLevel p_level, const std::string& p_message) {
    OS* os = OS::singleton_ptr();
    const uint32_t thread_id = thread::getThreadId();
    std::string thread_info;
    if (thread_id) {
        thread_info = std::format(" (thread id: {})", thread_id);
    }
    auto now = floor<std::chrono::seconds>(std::chrono::system_clock::now());
    std::string message_with_detail = std::format("[{:%H:%M:%S}]{} {}\n", now, thread_info, p_message);
    if (os) [[likely]] {
        os->print(p_level, message_with_detail);
    } else {
        StdLogger logger;
        logger.print(p_level, message_with_detail);
    }
}

}  // namespace my
