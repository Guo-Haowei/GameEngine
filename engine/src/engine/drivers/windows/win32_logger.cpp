#include "win32_logger.h"

#include "engine/core/base/ring_buffer.h"
#include "engine/drivers/windows/win32_prerequisites.h"

namespace my {

static WORD FindColorAttribute(LogLevel p_level) {
    switch (p_level) {
#define LOG_LEVEL_COLOR(LEVEL, TAG, ANSI, WINCOLOR) \
    case LEVEL:                                     \
        return WINCOLOR;
        LOG_LEVEL_COLOR_LIST
#undef LOG_LEVEL_COLOR
        default:
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
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
