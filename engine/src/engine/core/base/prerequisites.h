#pragma once
#include <cassert>

namespace my {

#if USING(DEBUG_BUILD)
constexpr inline bool IS_DEBUG_BUILD = true;
#else
constexpr inline bool IS_DEBUG_BUILD = false;
#endif

inline void assert_out_of_range() { assert(false && "index out of range"); }

constexpr std::size_t check_out_of_range(size_t i, size_t range) {
    return i < range ? i : (assert_out_of_range(), i);
}

constexpr std::size_t check_out_of_range_if_debug(size_t i, size_t range) {
    return IS_DEBUG_BUILD ? check_out_of_range(i, range) : i;
}

}  // namespace my
