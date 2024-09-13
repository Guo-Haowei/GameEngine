#include "error_macros.h"

#include <windows.h>

#include "core/os/os.h"

namespace my {

IntrusiveList<ErrorHandlerListNode> s_error_handlers;

void globalLock() {}
void globalUnlock() {}

void breakIfDebug() {
#if USING(DEBUG_BUILD)
    if (IsDebuggerPresent()) {
        __debugbreak();
    }
#endif
}

bool addErrorHandler(ErrorHandler* p_handler) {
    // if the handler already exists, remove it
    removeErrorHandler(p_handler);

    globalLock();
    s_error_handlers.node_push_front(p_handler);
    globalUnlock();
    return true;
}

bool removeErrorHandler(const ErrorHandler* p_handler) {
    globalLock();
    s_error_handlers.node_remove(p_handler);
    globalUnlock();
    return true;
}

void reportErrorImpl(std::string_view p_function,
                     std::string_view p_file,
                     int p_line,
                     std::string_view p_error,
                     std::string_view p_detail) {
    std::string extra;
    if (!p_detail.empty()) {
        extra = std::format("\nDetail: {}", p_detail);
    }

    auto message = std::format("ERROR: {}{}\n    at {} ({}:{})\n", p_error, extra, p_function, p_file, p_line);
    if (auto os = OS::singleton_ptr(); os) {
        os->print(LOG_LEVEL_ERROR, message);
    } else {
        fprintf(stderr, "%s", message.c_str());
    }

    globalLock();

    for (auto& handler : s_error_handlers) {
        handler.error_func(handler.user_data, p_function, p_file, p_line, p_error);
    }

    globalUnlock();
}

void reportErrorIndexImpl(std::string_view p_function,
                          std::string_view p_file,
                          int p_line,
                          std::string_view p_prefix,
                          int64_t p_index,
                          int64_t p_bound,
                          std::string_view p_index_string,
                          std::string_view p_bound_string,
                          std::string_view p_detail) {
    auto error2 = std::format("{}Index {} = {} is out of bounds ({} = {}).", p_prefix, p_index_string, p_index, p_bound_string, p_bound);

    reportErrorImpl(p_function, p_file, p_line, error2, p_detail);
}

}  // namespace my
