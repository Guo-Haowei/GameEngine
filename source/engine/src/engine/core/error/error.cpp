#include "error.h"

namespace my {

const char* ErrorToString(ErrorCode p_error) {
    static const char* s_error_names[] = {
#define ERROR_CODE(NAME) #NAME,
        ERROR_LIST
#undef ERROR_CODE
    };

    static_assert(array_length(s_error_names) == ERR_COUNT);

    return s_error_names[static_cast<int>(p_error)];
}

}  // namespace my