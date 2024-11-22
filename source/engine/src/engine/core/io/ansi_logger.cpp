#include "ansi_logger.h"

#include "core/base/ring_buffer.h"

namespace my {

void AnsiLogger::Print(LogLevel p_level, std::string_view p_message) {
    const char* color = "\033[0m";
    switch (p_level) {
        case my::LOG_LEVEL_VERBOSE:
            color = "\033[90m";
            break;
        case my::LOG_LEVEL_OK:
            color = "\033[92m";
            break;
        case my::LOG_LEVEL_WARN:
            color = "\033[33m";
            break;
        case my::LOG_LEVEL_ERROR:
            color = "\033[91m";
            break;
        case my::LOG_LEVEL_FATAL:
            color = "\033[101;30m";
            break;
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
