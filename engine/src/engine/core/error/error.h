#pragma once

namespace my {

class IStringBuilder;

#define ERROR_LIST                            \
    ERROR_CODE(OK)                            \
    ERROR_CODE(FAILURE)                       \
    ERROR_CODE(ERR_UNAVAILABLE)               \
    ERROR_CODE(ERR_UNCONFIGURED)              \
    ERROR_CODE(ERR_UNAUTHORIZED)              \
    ERROR_CODE(ERR_PARAMETER_RANGE_ERROR)     \
    ERROR_CODE(ERR_OUT_OF_MEMORY)             \
    ERROR_CODE(ERR_FILE_NOT_FOUND)            \
    ERROR_CODE(ERR_FILE_BAD_DRIVE)            \
    ERROR_CODE(ERR_FILE_BAD_PATH)             \
    ERROR_CODE(ERR_FILE_NO_PERMISSION)        \
    ERROR_CODE(ERR_FILE_ALREADY_IN_USE)       \
    ERROR_CODE(ERR_FILE_CANT_OPEN)            \
    ERROR_CODE(ERR_FILE_CANT_WRITE)           \
    ERROR_CODE(ERR_FILE_CANT_READ)            \
    ERROR_CODE(ERR_FILE_UNRECOGNIZED)         \
    ERROR_CODE(ERR_FILE_CORRUPT)              \
    ERROR_CODE(ERR_FILE_MISSING_DEPENDENCIES) \
    ERROR_CODE(ERR_FILE_EOF)                  \
    ERROR_CODE(ERR_CANT_OPEN)                 \
    ERROR_CODE(ERR_CANT_CREATE)               \
    ERROR_CODE(ERR_QUERY_FAILED)              \
    ERROR_CODE(ERR_ALREADY_IN_USE)            \
    ERROR_CODE(ERR_LOCKED)                    \
    ERROR_CODE(ERR_TIMEOUT)                   \
    ERROR_CODE(ERR_CANT_CONNECT)              \
    ERROR_CODE(ERR_CANT_RESOLVE)              \
    ERROR_CODE(ERR_CONNECTION_ERROR)          \
    ERROR_CODE(ERR_CANT_ACQUIRE_RESOURCE)     \
    ERROR_CODE(ERR_CANT_FORK)                 \
    ERROR_CODE(ERR_INVALID_DATA)              \
    ERROR_CODE(ERR_INVALID_INDEX)             \
    ERROR_CODE(ERR_INVALID_PARAMETER)         \
    ERROR_CODE(ERR_ALREADY_EXISTS)            \
    ERROR_CODE(ERR_DOES_NOT_EXIST)            \
    ERROR_CODE(ERR_DATABASE_CANT_READ)        \
    ERROR_CODE(ERR_DATABASE_CANT_WRITE)       \
    ERROR_CODE(ERR_COMPILATION_FAILED)        \
    ERROR_CODE(ERR_METHOD_NOT_FOUND)          \
    ERROR_CODE(ERR_LINK_FAILED)               \
    ERROR_CODE(ERR_SCRIPT_FAILED)             \
    ERROR_CODE(ERR_CYCLIC_LINK)               \
    ERROR_CODE(ERR_INVALID_DECLARATION)       \
    ERROR_CODE(ERR_DUPLICATE_SYMBOL)          \
    ERROR_CODE(ERR_PARSE_ERROR)               \
    ERROR_CODE(ERR_BUSY)                      \
    ERROR_CODE(ERR_SKIP)                      \
    ERROR_CODE(ERR_HELP)                      \
    ERROR_CODE(ERR_BUG)

enum class ErrorCode : uint16_t {
#define ERROR_CODE(ENUM) ENUM,
    ERROR_LIST
#undef ERROR_CODE
        COUNT,
};

const char* ErrorToString(ErrorCode p_error);

struct InternalErrorBase {
    InternalErrorBase(std::string_view p_filepath, std::string_view p_function, int p_line)
        : filepath(p_filepath), function(p_function), line(p_line) {}

    std::string_view filepath;
    std::string_view function;
    int line;
};

template<typename T>
struct InternalError : public InternalErrorBase {
public:
    using Ref = std::shared_ptr<InternalError<T>>;

    InternalError(std::string_view p_filepath,
                  std::string_view p_function,
                  int p_line,
                  const T& p_value)
        : InternalErrorBase(p_filepath, p_function, p_line), value(p_value) {}

    template<typename... Args>
    InternalError(std::string_view p_filepath,
                  std::string_view p_function,
                  int p_line,
                  const T& p_value,
                  std::format_string<Args...> p_format,
                  Args&&... p_args)
        : InternalError(p_filepath, p_function, p_line, p_value) {
        message = std::format(p_format, std::forward<Args>(p_args)...);
    }

    InternalError(std::string_view p_filepath,
                  std::string_view p_function,
                  int p_line,
                  Ref& p_next)
        : InternalErrorBase(p_filepath, p_function, p_line) {
        // @TODO: check overflow?
        next = p_next;
        depth = next->depth + 1;
        value = next->value;
    }

    T value{};
    // @TODO: union?
    std::string message;
    Ref next{ nullptr };
    int depth{ 0 };
};

template<typename T>
[[nodiscard]] static constexpr inline auto _create_error_arg_1(std::string_view p_file,
                                                               std::string_view p_function,
                                                               int p_line,
                                                               T p_value) {
    return std::unexpected(std::shared_ptr<InternalError<T>>(new InternalError<T>(p_file, p_function, p_line, p_value)));
};

template<typename T>
[[nodiscard]] static constexpr inline auto _create_error_arg_1(std::string_view p_file,
                                                               std::string_view p_function,
                                                               int p_line,
                                                               std::shared_ptr<InternalError<T>>& p_error) {
    return std::unexpected(std::shared_ptr<InternalError<T>>(new InternalError<T>(p_file, p_function, p_line, p_error)));
};

template<typename T, typename... Args>
[[nodiscard]] static constexpr inline auto _create_error_arg_2_plus(std::string_view p_file,
                                                                    std::string_view p_function,
                                                                    int p_line,
                                                                    T p_value,
                                                                    std::format_string<Args...> p_format,
                                                                    Args&&... p_args) {
    return std::unexpected(std::shared_ptr<InternalError<T>>(new InternalError<T>(p_file, p_function, p_line, p_value, p_format, std::forward<Args>(p_args)...)));
};

#define HBN_ERROR_1(_1)                             ::my::_create_error_arg_1<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1)
#define HBN_ERROR_2(_1, _2)                         ::my::_create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2)
#define HBN_ERROR_3(_1, _2, _3)                     ::my::_create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2, _3)
#define HBN_ERROR_4(_1, _2, _3, _4)                 ::my::_create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2, _3, _4)
#define HBN_ERROR_5(_1, _2, _3, _4, _5)             ::my::_create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2, _3, _4, _5)
#define HBN_ERROR_6(_1, _2, _3, _4, _5, _6)         ::my::_create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2, _3, _4, _5, _6)
#define HBN_ERROR_7(_1, _2, _3, _4, _5, _6, _7)     ::my::_create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2, _3, _4, _5, _6, _7)
#define HBN_ERROR_8(_1, _2, _3, _4, _5, _6, _7, _8) ::my::_create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2, _3, _4, _5, _6, _7, _8)
#define HBN_ERROR(...)                              HBN_MACRO_EXPAND(HBN_GET_MACRO_8(__VA_ARGS__, HBN_ERROR_8, HBN_ERROR_7, HBN_ERROR_6, HBN_ERROR_5, HBN_ERROR_4, HBN_ERROR_3, HBN_ERROR_2, HBN_ERROR_1)(__VA_ARGS__))

using Error = InternalError<ErrorCode>;
using ErrorRef = std::shared_ptr<Error>;

template<typename T>
using Result = std::expected<T, ErrorRef>;

IStringBuilder& operator<<(IStringBuilder& p_stream, const ErrorRef& p_error);

}  // namespace my
