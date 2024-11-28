#include "string_builder.h"

namespace my {

// @TODO: refactor
// constexpr std::string_view find_last_slash(std::string_view p_string, char p_char) {
//    std::string_view result = p_string;
//
//    // Iterate over the string in reverse order
//    for (size_t i = p_string.size() - 1; i > 0; --i) {
//        if (p_string[i] == p_char) {
//            result = std::string_view(p_string.data() + i, p_string.size() - i);
//            break;
//        }
//    }
//
//    return result;
//}

}  // namespace my
