#include "error.h"

namespace my {

const char* errorToString(ErrorCode p_error) {
    static const char* s_error_names[] = {
#define ERROR_CODE_NAME
#include "error_list.inl.h"
#undef ERROR_CODE_NAME
    };

    static_assert(array_length(s_error_names) == ERR_COUNT);

    return s_error_names[static_cast<int>(p_error)];
}

}  // namespace my