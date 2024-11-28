#include "ansi_logger.h"

#include "engine/core/base/ring_buffer.h"

namespace my {

void AnsiLogger::Print(LogLevel p_level, std::string_view p_message) {
    const char* color = "\033[0m";
    switch (p_level) {
#define LOG_LEVEL_COLOR(LEVEL, TAG, ANSI, WINCOLOR) \
    case LEVEL:                                     \
        color = ANSI;                               \
        break;
        LOG_LEVEL_COLOR_LIST
#undef LOG_LEVEL_COLOR
        default:
            break;
    }

    // @TODO: stderr vs stdout
    FILE* file = stdout;
    fflush(file);
    fprintf(file, "%s%.*s\033[0m", color, static_cast<int>(p_message.length()), p_message.data());
    fflush(file);
}

}  // namespace my
