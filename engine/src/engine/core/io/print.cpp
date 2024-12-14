#include "print.h"

#include "engine/core/io/print.h"
#include "engine/core/os/os.h"
#include "engine/core/os/threads.h"

namespace my {

void PrintImpl(LogLevel p_level, const std::string& p_message) {
    OS* os = OS::GetSingletonPtr();
    if (os) [[likely]] {
        os->Print(p_level, p_message);
    } else {
        StdLogger logger;
        logger.Print(p_level, p_message);
    }
}

void LogImpl(LogLevel p_level, const std::string& p_message) {
    OS* os = OS::GetSingletonPtr();
    const uint32_t thread_id = thread::GetThreadId();
    std::string thread_info;
    if (thread_id) {
        thread_info = std::format(" (thread id: {})", thread_id);
    }
    auto now = floor<std::chrono::seconds>(std::chrono::system_clock::now());
    std::string message_with_detail = std::format("[{:%H:%M:%S}]{} {}\n", now, thread_info, p_message);
    if (os) [[likely]] {
        os->Print(p_level, message_with_detail);
    } else {
        printf("%s\n", p_message.c_str());
    }
}

}  // namespace my
