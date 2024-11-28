#include <unistd.h>

#include "engine/core/io/ansi_logger.h"
#include "engine/core/io/file_access_unix.h"
#include "engine/core/os/os.h"

namespace my {

void OS::Initialize() {
    FileAccess::MakeDefault<FileAccessUnix>(FileAccess::ACCESS_RESOURCE);
    FileAccess::MakeDefault<FileAccessUnix>(FileAccess::ACCESS_USERDATA);
    FileAccess::MakeDefault<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);

    if (IsAnsiSupported()) {
        AddLogger(std::make_shared<AnsiLogger>());
    } else {
        AddLogger(std::make_shared<StdLogger>());
    }
}

bool IsAnsiSupported() {
    if (!isatty(STDOUT_FILENO)) {
        return false;  // Output is not a terminal
    }

    const char* term_cstring = std::getenv("TERM");
    if (term_cstring == nullptr) {
        return false;  // TERM is not set
    }

    std::string term(term_cstring);

    return (term.find("xterm") != std::string::npos ||
            term.find("vt100") != std::string::npos ||
            term.find("screen") != std::string::npos ||
            term.find("ansi") != std::string::npos);
}

}  // namespace my
