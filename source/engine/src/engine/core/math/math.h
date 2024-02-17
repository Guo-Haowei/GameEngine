#pragma once

#ifdef BIT
#undef BIT
#endif
#define BIT(x) my::math::bit(x)

namespace my::math {

constexpr inline uint64_t bit(uint64_t a) {
    return (1llu << a) >> 1llu;
}

template<typename T, class = typename std::enable_if<std::is_integral<T>::value>::type>
constexpr inline T align(T size, T alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// TODO: smaller size

constexpr unsigned int log_two(unsigned int x) {
    return x == 1 ? 0 : 1 + log_two(x >> 1);
}

constexpr bool is_power_of_two(unsigned int x) {
    return (x & (x - 1)) == 0;
}

constexpr inline uint32_t next_power_of_two(uint32_t x) {
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
}

}  // namespace my::math
