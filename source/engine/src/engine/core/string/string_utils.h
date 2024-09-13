#pragma once

namespace my {

class StringUtils {
public:
    static void replaceFirst(std::string& p_string, std::string_view p_pattern, std::string_view p_replacement);
};

}  // namespace my