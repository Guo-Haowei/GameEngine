#include "error.h"

#include "core/string/string_builder.h"

namespace my {

const char* ErrorToString(ErrorCode p_error) {
    static const char* s_errorNames[] = {
#define ERROR_CODE(NAME) #NAME,
        ERROR_LIST
#undef ERROR_CODE
    };

    static_assert(array_length(s_errorNames) == ERR_COUNT);

    return s_errorNames[static_cast<int>(p_error)];
}

IStringBuilder& operator<<(IStringBuilder& p_stream, const ErrorRef& p_error) {
    std::vector<Error*> stack;
    Error* cursor = p_error.get();
    while (cursor) {
        stack.emplace_back(cursor);
        cursor = cursor->next.get();
    }

    Error* back = stack.back();
    p_stream.Append(std::format("Error \"{}\" occured, \"{}\".\n", ErrorToString(back->value), back->message));

    bool trace = stack.size() > 1;
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        const Error& error = *(*it);

        auto stack_trace = std::format("    at {} ({}:{})\n",
                                       error.function,
                                       error.filepath,
                                       error.line);
        p_stream.Append(stack_trace);

        if (trace) {
            p_stream.Append("  stack backtrace:\n");
            trace = false;
        }
    }

    return p_stream;
}

}  // namespace my