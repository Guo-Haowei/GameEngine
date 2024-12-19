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

template<Arithmetic T>
constexpr T Min(const T& p_lhs, const T& p_rhs) {
    return p_lhs < p_rhs ? p_lhs : p_rhs;
}

template<Arithmetic T>
constexpr T Max(const T& p_lhs, const T& p_rhs) {
    return p_lhs > p_rhs ? p_lhs : p_rhs;
}

template<Arithmetic T, int N>
constexpr Vector<T, N> Min(const Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    constexpr int dim = sizeof(p_lhs) / sizeof(p_lhs.x);
    Vector<T, N> result;
    for (int i = 0; i < dim; ++i) {
        result[i] = Min(p_lhs[i], p_rhs[i]);
    }
    return result;
}

template<Arithmetic T, int N>
constexpr Vector<T, N> Max(const Vector<T, N>& p_lhs, const Vector<T, N>& p_rhs) {
    constexpr int dim = sizeof(p_lhs) / sizeof(p_lhs.x);
    Vector<T, N> result;
    for (int i = 0; i < dim; ++i) {
        result[i] = Max(p_lhs[i], p_rhs[i]);
    }
    return result;
}

template<Arithmetic T, int N>
constexpr Vector<T, N> Clamp(const Vector<T, N>& p_value, const Vector<T, N>& p_min, const Vector<T, N>& p_max) {
    return Max(p_min, Min(p_value, p_max));
}

template<Arithmetic T, int N>
constexpr Vector<T, N> Lerp(const Vector<T, N>& p_x, const Vector<T, N>& p_y, float p_s) {
    return (static_cast<T>(1) - p_s) * p_x + p_s * p_y;
}

}  // namespace my::math
