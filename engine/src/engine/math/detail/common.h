#pragma once
#include "forward.h"

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

namespace my {

template<typename T = float>
    requires std::is_floating_point_v<T>
constexpr inline T Pi() {
    return static_cast<T>(3.14159265358979323846264338327950288);
}

template<typename T = float>
    requires std::is_floating_point_v<T>
constexpr inline T HalfPi() {
    return static_cast<T>(0.5) * Pi();
}

template<typename T = float>
    requires std::is_floating_point_v<T>
constexpr inline T TwoPi() {
    return static_cast<T>(2) * Pi();
}

template<typename T = float>
    requires std::is_floating_point_v<T>
constexpr inline T Epsilon() {
    return std::numeric_limits<T>::epsilon();
}

template<typename T>
    requires std::is_floating_point_v<T>
constexpr inline T Radians(const T& p_degrees) {
    return p_degrees * static_cast<T>(0.01745329251994329576923690768489);
}

template<typename T>
    requires std::is_floating_point_v<T>
constexpr inline T Degrees(const T& p_radians) {
    return p_radians * static_cast<T>(57.295779513082320876798154814105);
}

template<Arithmetic T>
constexpr inline T min(const T& p_lhs, const T& p_rhs) {
    return p_lhs < p_rhs ? p_lhs : p_rhs;
}

template<Arithmetic T>
constexpr inline T max(const T& p_lhs, const T& p_rhs) {
    return p_lhs > p_rhs ? p_lhs : p_rhs;
}

template<Arithmetic T>
constexpr inline T abs(const T& p_lhs) {
    return std::abs(p_lhs);
}

template<Arithmetic T>
constexpr inline T clamp(const T& p_value, const T& p_min, const T& p_max) {
    return max(p_min, min(p_value, p_max));
}

}  // namespace my
