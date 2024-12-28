#pragma once

#ifdef BIT
#undef BIT
#endif
#define BIT(x) my::Bit(x)

namespace my {

constexpr inline uint64_t Bit(uint64_t p_a) {
    return (1llu << p_a) >> 1llu;
}

template<typename T, class = typename std::enable_if<std::is_integral<T>::value>::type>
constexpr inline T Align(T p_size, T p_alignment) {
    return (p_size + p_alignment - 1) & ~(p_alignment - 1);
}

constexpr unsigned int LogTwo(unsigned int p_x) {
    return p_x == 1 ? 0 : 1 + LogTwo(p_x >> 1);
}

constexpr bool IsPowerOfTwo(unsigned int p_x) {
    return (p_x & (p_x - 1)) == 0;
}

constexpr inline uint32_t NextPowerOfTwo(uint32_t p_x) {
    --p_x;
    p_x |= p_x >> 1;
    p_x |= p_x >> 2;
    p_x |= p_x >> 4;
    p_x |= p_x >> 8;
    p_x |= p_x >> 16;
    return ++p_x;
}

constexpr inline int CeilingDivision(int p_dividend, int p_divisor) {
    return (p_dividend + p_divisor - 1) / p_divisor;
}

}  // namespace my
