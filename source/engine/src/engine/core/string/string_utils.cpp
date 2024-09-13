#include "string_utils.h"

namespace my {

void StringUtils::replaceFirst(std::string& p_string, std::string_view p_pattern, std::string_view p_replacement) {
    p_string.replace(0, p_pattern.size(), p_replacement);
}

}  // namespace my