#pragma once

#ifdef BIT
#undef BIT
#endif
#define BIT(x) my::math::bit(x)

namespace my::math {

constexpr inline uint64_t bit(uint64_t p_a) {
    return (1llu << p_a) >> 1llu;
}

template<typename T, class = typename std::enable_if<std::is_integral<T>::value>::type>
constexpr inline T align(T p_size, T p_alignment) {
    return (p_size + p_alignment - 1) & ~(p_alignment - 1);
}

constexpr unsigned int logTwo(unsigned int p_x) {
    return p_x == 1 ? 0 : 1 + logTwo(p_x >> 1);
}

constexpr bool isPowerOfTwo(unsigned int p_x) {
    return (p_x & (p_x - 1)) == 0;
}

constexpr inline uint32_t nextPowerOfTwo(uint32_t p_x) {
    --p_x;
    p_x |= p_x >> 1;
    p_x |= p_x >> 2;
    p_x |= p_x >> 4;
    p_x |= p_x >> 8;
    p_x |= p_x >> 16;
    return ++p_x;
}

constexpr inline int ceilingDivision(int p_dividend, int p_divisor) {
    return (p_dividend + p_divisor - 1) / p_divisor;
}

}  // namespace my::math
