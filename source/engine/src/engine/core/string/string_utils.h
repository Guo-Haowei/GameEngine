#pragma once

namespace my {

class StringUtils {
public:
    static bool stringEqual(const char* p_str1, const char* p_str2);

    static void replaceFirst(std::string& p_string, std::string_view p_pattern, std::string_view p_replacement);
};

}  // namespace my
