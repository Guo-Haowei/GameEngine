#include "error_macros.h"

#include "engine/core/os/os.h"
#include "engine/drivers/windows/win32_prerequisites.h"

namespace my {

IntrusiveList<ErrorHandlerListNode> s_errorHandlers;

void GlobalLock() {}
void GlobalUnlock() {}

void BreakIfDebug() {
#if USING(PLATFORM_WINDOWS)
    if (IsDebuggerPresent()) {
        GENERATE_TRAP();
    }
#endif
}

bool AddErrorHandler(ErrorHandler* p_handler) {
    // if the handler already exists, remove it
    RemoveErrorHandler(p_handler);

    GlobalLock();
    s_errorHandlers.node_push_front(p_handler);
    GlobalUnlock();
    return true;
}

bool RemoveErrorHandler(const ErrorHandler* p_handler) {
    GlobalLock();
    s_errorHandlers.node_remove(p_handler);
    GlobalUnlock();
    return true;
}

void ReportErrorImpl(std::string_view p_function,
                     std::string_view p_file,
                     int p_line,
                     std::string_view p_error,
                     std::string_view p_detail) {
    std::string extra;
    if (!p_detail.empty()) {
        extra = std::format("\nDetail: {}", p_detail);
    }

    auto message = std::format("ERROR: {}{}\n    at {} ({}:{})\n",
                               p_error,
                               extra,
                               p_function,
                               p_file,
                               p_line);
    if (auto os = OS::GetSingletonPtr(); os) {
        os->Print(LOG_LEVEL_ERROR, message);
    } else {
        fprintf(stderr, "%s", message.c_str());
    }

    GlobalLock();

    for (auto& handler : s_errorHandlers) {
        handler.errorFunc(handler.userdata, p_function, p_file, p_line, p_error);
    }

    GlobalUnlock();
}

void ReportErrorIndexImpl(std::string_view p_function,
                          std::string_view p_file,
                          int p_line,
                          std::string_view p_prefix,
                          int64_t p_index,
                          int64_t p_bound,
                          std::string_view p_index_string,
                          std::string_view p_bound_string,
                          std::string_view p_detail) {
    auto error2 = std::format("{}Index {} = {} is out of bounds ({} = {}).", p_prefix, p_index_string, p_index, p_bound_string, p_bound);

    ReportErrorImpl(p_function, p_file, p_line, error2, p_detail);
}

}  // namespace my
