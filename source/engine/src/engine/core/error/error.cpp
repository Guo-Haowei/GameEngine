#include "error.h"

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

}  // namespace my