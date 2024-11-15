#include "std_logger.h"

#include "core/base/ring_buffer.h"
#include "drivers/windows/win32_prerequisites.h"

namespace my {

#if USING(PLATFORM_WINDOWS)
static WORD FindColorAttribute(LogLevel p_level) {
    switch (p_level) {
        case LOG_LEVEL_OK:
            return FOREGROUND_GREEN;
        case LOG_LEVEL_WARN:
            return FOREGROUND_RED | FOREGROUND_GREEN;
        case LOG_LEVEL_ERROR:
            [[fallthrough]];
        case LOG_LEVEL_FATAL:
            return FOREGROUND_RED;
        default:
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
}
#endif

void StdLogger::Print(LogLevel p_level, std::string_view p_message) {
    unused(p_level);
#if USING(PLATFORM_WINDOWS)
    const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    const WORD new_color = FindColorAttribute(p_level);
#endif

    // @TODO: stderr vs stdout

    FILE* file = stdout;
    fflush(file);

#if USING(PLATFORM_WINDOWS)
    m_consoleMutex.lock();
    GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
    const WORD old_color_attrs = buffer_info.wAttributes;
    SetConsoleTextAttribute(stdout_handle, new_color);
    fprintf(file, "%.*s", static_cast<int>(p_message.length()), p_message.data());
    SetConsoleTextAttribute(stdout_handle, old_color_attrs);
    fflush(file);
    m_consoleMutex.unlock();
#else
    fprintf(file, "%.*s", static_cast<int>(p_message.length()), p_message.data());
    fflush(file);
#endif
}

void DebugConsoleLogger::Print(LogLevel p_level, std::string_view p_message) {
    unused(p_level);
    unused(p_message);

#if USING(PLATFORM_WINDOWS)
    std::string message{ p_message };
    OutputDebugStringA(message.c_str());
#endif
}

}  // namespace my
