#pragma once
#include "vector.h"

namespace my {

template<Arithmetic T, int N>
constexpr bool operator==(const Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    for (int i = 0; i < N; ++i) {
        if (p_lhs[i] != p_rhs[i]) {
            return false;
        }
    }
    return true;
}

#define VECTOR_OPERATOR_VEC_VEC(DEST, OP, LHS, RHS)          \
    do {                                                     \
        constexpr int DIM = sizeof(LHS) / sizeof(LHS.x);     \
        DEST.x = LHS.x OP RHS.x;                             \
        DEST.y = LHS.y OP RHS.y;                             \
        if constexpr (DIM >= 3) { DEST.z = LHS.z OP RHS.z; } \
        if constexpr (DIM >= 4) { DEST.w = LHS.w OP RHS.w; } \
    } while (0)

#define VECTOR_OPERATOR_VEC_SCALAR(DEST, OP, LHS, RHS)     \
    do {                                                   \
        constexpr int DIM = sizeof(LHS) / sizeof(LHS.x);   \
        DEST.x = LHS.x OP RHS;                             \
        DEST.y = LHS.y OP RHS;                             \
        if constexpr (DIM >= 3) { DEST.z = LHS.z OP RHS; } \
        if constexpr (DIM >= 4) { DEST.w = LHS.w OP RHS; } \
    } while (0)

#define VECTOR_OPERATOR_SCALAR_VEC(DEST, OP, LHS, RHS)     \
    do {                                                   \
        constexpr int DIM = sizeof(RHS) / sizeof(RHS.x);   \
        DEST.x = LHS OP RHS.x;                             \
        DEST.y = LHS OP RHS.y;                             \
        if constexpr (DIM >= 3) { DEST.z = LHS OP RHS.z; } \
        if constexpr (DIM >= 4) { DEST.w = LHS OP RHS.w; } \
    } while (0)

#pragma region VECTOR_MATH_ADD
template<Arithmetic T, int N>
constexpr Vector<T, N> operator+(const Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    Vector<T, N> result;
    VECTOR_OPERATOR_VEC_VEC(result, +, p_lhs, p_rhs);
    return result;
}

template<Arithmetic T, int N, Arithmetic U>
constexpr Vector<T, N> operator+(const Vector<T, N>& p_lhs, const U& p_rhs) {
    Vector<T, N> result;
    VECTOR_OPERATOR_VEC_SCALAR(result, +, p_lhs, p_rhs);
    return result;
}

template<Arithmetic T, int N, Arithmetic U>
constexpr Vector<T, N> operator+(const U& p_lhs, const Vector<T, N>& p_rhs) {
    Vector<T, N> result;
    VECTOR_OPERATOR_SCALAR_VEC(result, +, p_lhs, p_rhs);
    return result;
}

template<Arithmetic T, int N>
constexpr Vector<T, N>& operator+=(Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    VECTOR_OPERATOR_VEC_VEC(p_lhs, +, p_lhs, p_rhs);
    return p_lhs;
}

template<Arithmetic T, int N, Arithmetic U>
constexpr Vector<T, N>& operator+=(Vector<T, N>& p_lhs, const U& p_rhs) {
    VECTOR_OPERATOR_VEC_SCALAR(p_lhs, +, p_lhs, p_rhs);
    return p_lhs;
}
#pragma endregion VECTOR_MATH_ADD

#pragma region VECTOR_MATH_SUB
template<Arithmetic T, int N>
constexpr Vector<T, N> operator-(const Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    Vector<T, N> result;
    VECTOR_OPERATOR_VEC_VEC(result, -, p_lhs, p_rhs);
    return result;
}

template<Arithmetic T, int N, Arithmetic U>
constexpr Vector<T, N> operator-(const Vector<T, N>& p_lhs, const U& p_rhs) {
    Vector<T, N> result;
    VECTOR_OPERATOR_VEC_SCALAR(result, -, p_lhs, p_rhs);
    return result;
}

template<Arithmetic T, int N, Arithmetic U>
constexpr Vector<T, N> operator-(const U& p_lhs, const Vector<T, N>& p_rhs) {
    Vector<T, N> result;
    VECTOR_OPERATOR_SCALAR_VEC(result, -, p_lhs, p_rhs);
    return result;
}

template<Arithmetic T, int N>
constexpr Vector<T, N>& operator-=(Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    VECTOR_OPERATOR_VEC_VEC(p_lhs, -, p_lhs, p_rhs);
    return p_lhs;
}

template<Arithmetic T, int N, Arithmetic U>
constexpr Vector<T, N>& operator-=(Vector<T, N>& p_lhs, const U& p_rhs) {
    VECTOR_OPERATOR_VEC_SCALAR(p_lhs, -, p_lhs, p_rhs);
    return p_lhs;
}
#pragma endregion VECTOR_MATH_SUB

#pragma region VECTOR_MATH_MUL
template<Arithmetic T, int N>
constexpr Vector<T, N> operator*(const Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    Vector<T, N> result;
    VECTOR_OPERATOR_VEC_VEC(result, *, p_lhs, p_rhs);
    return result;
}

template<Arithmetic T, int N, Arithmetic U>
constexpr Vector<T, N> operator*(const Vector<T, N>& p_lhs, const U& p_rhs) {
    Vector<T, N> result;
    VECTOR_OPERATOR_VEC_SCALAR(result, *, p_lhs, p_rhs);
    return result;
}

template<Arithmetic T, int N, Arithmetic U>
constexpr Vector<T, N> operator*(const U& p_lhs, const Vector<T, N>& p_rhs) {
    Vector<T, N> result;
    VECTOR_OPERATOR_SCALAR_VEC(result, *, p_lhs, p_rhs);
    return result;
}

template<Arithmetic T, int N>
constexpr Vector<T, N>& operator*=(Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    VECTOR_OPERATOR_VEC_VEC(p_lhs, *, p_lhs, p_rhs);
    return p_lhs;
}

template<Arithmetic T, int N, Arithmetic U>
constexpr Vector<T, N>& operator*=(Vector<T, N>& p_lhs, const U& p_rhs) {
    VECTOR_OPERATOR_VEC_SCALAR(p_lhs, *, p_lhs, p_rhs);
    return p_lhs;
}
#pragma endregion VECTOR_MATH_MUL

#pragma region VECTOR_MATH_DIV
template<Arithmetic T, int N>
constexpr Vector<T, N> operator/(const Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    Vector<T, N> result;
    VECTOR_OPERATOR_VEC_VEC(result, /, p_lhs, p_rhs);
    return result;
}

template<Arithmetic T, int N, Arithmetic U>
constexpr Vector<T, N> operator/(const Vector<T, N>& p_lhs, const U& p_rhs) {
    Vector<T, N> result;
    VECTOR_OPERATOR_VEC_SCALAR(result, /, p_lhs, p_rhs);
    return result;
}

template<Arithmetic T, int N, Arithmetic U>
constexpr Vector<T, N> operator/(const U& p_lhs, const Vector<T, N>& p_rhs) {
    Vector<T, N> result;
    VECTOR_OPERATOR_SCALAR_VEC(result, /, p_lhs, p_rhs);
    return result;
}

template<Arithmetic T, int N>
constexpr Vector<T, N>& operator/=(Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    VECTOR_OPERATOR_VEC_VEC(p_lhs, /, p_lhs, p_rhs);
    return p_lhs;
}

template<Arithmetic T, int N, Arithmetic U>
constexpr Vector<T, N>& operator/=(Vector<T, N>& p_lhs, const U& p_rhs) {
    VECTOR_OPERATOR_VEC_SCALAR(p_lhs, /, p_lhs, p_rhs);
    return p_lhs;
}
#pragma endregion VECTOR_MATH_DIV

}  // namespace my

namespace my::math {

// @TODO: refactor
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
constexpr inline T Min(const T& p_lhs, const T& p_rhs) {
    return p_lhs < p_rhs ? p_lhs : p_rhs;
}

template<Arithmetic T>
constexpr inline T Max(const T& p_lhs, const T& p_rhs) {
    return p_lhs > p_rhs ? p_lhs : p_rhs;
}

template<Arithmetic T>
constexpr inline T Abs(const T& p_lhs) {
    return std::abs(p_lhs);
}

template<Arithmetic T, int N>
constexpr inline Vector<T, N> Min(const Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    Vector<T, N> result;
    result.x = Min(p_lhs.x, p_rhs.x);
    result.y = Min(p_lhs.y, p_rhs.y);
    if constexpr (N >= 3) {
        result.z = Min(p_lhs.z, p_rhs.z);
    }
    if constexpr (N >= 4) {
        result.w = Min(p_lhs.w, p_rhs.w);
    }
    return result;
}

template<Arithmetic T, int N>
constexpr inline Vector<T, N> Max(const Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    Vector<T, N> result;
    result.x = Max(p_lhs.x, p_rhs.x);
    result.y = Max(p_lhs.y, p_rhs.y);
    if constexpr (N >= 3) {
        result.z = Max(p_lhs.z, p_rhs.z);
    }
    if constexpr (N >= 4) {
        result.w = Max(p_lhs.w, p_rhs.w);
    }
    return result;
}

template<Arithmetic T, int N>
constexpr inline Vector<T, N> Abs(const Vector<T, N>& p_lhs) {
    Vector<T, N> result;
    result.x = Abs(p_lhs.x);
    result.y = Abs(p_lhs.y);
    if constexpr (N >= 3) {
        result.z = Abs(p_lhs.z);
    }
    if constexpr (N >= 4) {
        result.w = Abs(p_lhs.w);
    }
    return result;
}

template<Arithmetic T>
constexpr inline T Clamp(const T& p_value, const T& p_min, const T& p_max) {
    return Max(p_min, Min(p_value, p_max));
}

template<Arithmetic T, int N>
constexpr inline Vector<T, N> Clamp(const Vector<T, N>& p_value, const Vector<T, N>& p_min, const Vector<T, N>& p_max) {
    return Max(p_min, Min(p_value, p_max));
}

template<FloatingPoint T, int N>
constexpr inline Vector<T, N> Lerp(const Vector<T, N>& p_x, const Vector<T, N>& p_y, float p_s) {
    return (static_cast<T>(1) - p_s) * p_x + p_s * p_y;
}

template<Arithmetic T, int N>
constexpr inline T Dot(const Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    Vector<T, N> tmp(p_lhs * p_rhs);
    T result = tmp.x + tmp.y;
    if constexpr (N >= 3) {
        result += tmp.z;
    }
    if constexpr (N >= 4) {
        result += tmp.w;
    }
    return result;
}

template<Arithmetic T, int N>
    requires(std::is_floating_point_v<T>)
constexpr inline T Length(const Vector<T, N>& p_lhs) {
    return std::sqrt(Dot(p_lhs, p_lhs));
}

template<Arithmetic T, int N>
    requires(std::is_floating_point_v<T>)
constexpr inline Vector<T, N> Normalize(const Vector<T, N>& p_lhs) {
    const auto inverse_length = static_cast<T>(1) / Length(p_lhs);
    return p_lhs * inverse_length;
}

template<Arithmetic T>
constexpr inline Vector<T, 3> Cross(const Vector<T, 3>& p_lhs, const Vector<T, 3>& p_rhs) {
    return Vector<T, 3>(
        p_lhs.y * p_rhs.z - p_rhs.y * p_lhs.z,
        p_lhs.z * p_rhs.x - p_rhs.z * p_lhs.x,
        p_lhs.x * p_rhs.y - p_rhs.x * p_lhs.y);
}

}  // namespace my::math
