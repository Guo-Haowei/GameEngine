#pragma once

namespace my {

enum ErrorCode : uint16_t {
#define ERROR_CODE_ENUM
#include "error_list.inl.h"
#undef ERROR_CODE_ENUM
    ERR_COUNT,
};

struct ErrorBase {
    ErrorBase(std::string_view p_filepath, std::string_view p_function, int p_line)
        : filepath(p_filepath), function(p_function), line(p_line) {}

    std::string_view filepath;
    std::string_view function;
    int line;
};

template<typename T>
class Error : public ErrorBase {
public:
    Error(std::string_view p_filepath,
          std::string_view p_function,
          int p_line,
          const T& p_value)
        : ErrorBase(p_filepath, p_function, p_line), m_value(p_value) {}

    template<typename... Args>
    Error(std::string_view p_filepath,
          std::string_view p_function,
          int p_line,
          const T& p_value,
          std::format_string<Args...> p_format,
          Args&&... p_args)
        : Error(p_filepath, p_function, p_line, p_value) {
        m_message = std::format(p_format, std::forward<Args>(p_args)...);
    }

    const T& getValue() const { return m_value; }

    const std::string& getMessage() const { return m_message; }

private:
    T m_value;
    std::string m_message;
};

#define VCT_ERROR(VALUE, ...) std::unexpected(::my::Error(__FILE__, __FUNCTION__, __LINE__, VALUE, ##__VA_ARGS__))
;
const char* errorToString(ErrorCode p_error);

}  // namespace my
