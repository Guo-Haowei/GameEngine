#include "string_utils.h"

namespace my {

bool StringUtils::StringEqual(const char* p_str1, const char* p_str2) {
    p_str1 = p_str1 ? p_str1 : "";
    p_str2 = p_str2 ? p_str2 : "";
    return strcmp(p_str1, p_str2) == 0;
}

void StringUtils::ReplaceFirst(std::string& p_string, std::string_view p_pattern, std::string_view p_replacement) {
    size_t pos = p_string.find(p_pattern);
    if (pos != std::string::npos) {
        p_string.replace(pos, p_pattern.size(), p_replacement);
    }
}

char* StringUtils::Strdup(const char* p_source) {
#if USING(PLATFORM_WINDOWS)
    return _strdup(p_source);
#elif USING(PLATFORM_APPLE)
    return strdup(p_source);
#else
#error Platform not supported
#endif
}

}  // namespace my
