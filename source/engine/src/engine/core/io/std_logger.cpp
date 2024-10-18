#include "std_logger.h"

#include "core/base/ring_buffer.h"

// @TODO: refactor
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace my {

static WORD find_color_attribute(LogLevel p_level) {
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

void StdLogger::print(LogLevel p_level, std::string_view p_message) {
    const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    const WORD new_color = find_color_attribute(p_level);

    // @TODO: stderr vs stdout

    FILE* file = stdout;
    fflush(file);

    m_console_mutex.lock();
    GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
    const WORD old_color_attrs = buffer_info.wAttributes;
    SetConsoleTextAttribute(stdout_handle, new_color);
    fprintf(file, "%.*s", static_cast<int>(p_message.length()), p_message.data());
    SetConsoleTextAttribute(stdout_handle, old_color_attrs);
    fflush(file);
    m_console_mutex.unlock();
}

void DebugConsoleLogger::print(LogLevel p_level, std::string_view p_message) {
    unused(p_level);

    std::string message{ p_message };
    OutputDebugStringA(message.c_str());
}

}  // namespace my
