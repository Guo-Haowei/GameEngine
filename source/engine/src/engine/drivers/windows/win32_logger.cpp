#include "win32_logger.h"

#include "core/base/ring_buffer.h"
#include "drivers/windows/win32_prerequisites.h"

namespace my {

static WORD FindColorAttribute(LogLevel p_level) {
    WORD color = FOREGROUND_INTENSITY;
    switch (p_level) {
        case LOG_LEVEL_OK:
            color |= FOREGROUND_GREEN;
            break;
        case LOG_LEVEL_WARN:
            color |= FOREGROUND_RED | FOREGROUND_GREEN;
            break;
        case LOG_LEVEL_ERROR:
            [[fallthrough]];
        case LOG_LEVEL_FATAL:
            color |= FOREGROUND_RED;
            break;
        default:
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
    return color;
}

void Win32Logger::Print(LogLevel p_level, std::string_view p_message) {
    const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    const WORD new_color = FindColorAttribute(p_level);

    // @TODO: stderr vs stdout
    FILE* file = stdout;
    fflush(file);

    m_consoleMutex.lock();
    GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
    const WORD old_color_attrs = buffer_info.wAttributes;
    SetConsoleTextAttribute(stdout_handle, new_color);
    fprintf(file, "%.*s", static_cast<int>(p_message.length()), p_message.data());
    SetConsoleTextAttribute(stdout_handle, old_color_attrs);
    fflush(file);
    m_consoleMutex.unlock();
}

}  // namespace my
