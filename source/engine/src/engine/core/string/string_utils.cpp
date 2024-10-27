#include "string_utils.h"

namespace my {

bool StringUtils::stringEqual(const char* p_str1, const char* p_str2) {
    p_str1 = p_str1 ? p_str1 : "";
    p_str2 = p_str2 ? p_str2 : "";
    return strcmp(p_str1, p_str2) == 0;
}

void StringUtils::replaceFirst(std::string& p_string, std::string_view p_pattern, std::string_view p_replacement) {
    size_t pos = p_string.find(p_pattern);
    if (pos != std::string::npos) {
        p_string.replace(pos, p_pattern.size(), p_replacement);
    }
}

}  // namespace my
