#pragma once
#include "vector.h"

namespace my {

template<typename T>
    requires VectorN<T>
constexpr bool operator==(const T& p_lhs, const T& p_rhs) {
    constexpr int dim = sizeof(p_lhs) / sizeof(p_lhs.x);
    for (int i = 0; i < dim; ++i) {
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
template<typename T>
    requires VectorN<T>
constexpr T operator+(const T& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_VEC(result, +, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires VectorN<T> && Arithmetic<U>
constexpr T operator+(const T& p_lhs, const U& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_SCALAR(result, +, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires VectorN<T> && Arithmetic<U>
constexpr T operator+(const U& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_SCALAR_VEC(result, +, p_lhs, p_rhs);
    return result;
}

template<typename T>
    requires VectorN<T>
constexpr T& operator+=(T& p_lhs, const T& p_rhs) {
    VECTOR_OPERATOR_VEC_VEC(p_lhs, +, p_lhs, p_rhs);
    return p_lhs;
}

template<typename T, typename U>
    requires VectorN<T> && Arithmetic<U>
constexpr T& operator+=(T& p_lhs, const U& p_rhs) {
    VECTOR_OPERATOR_VEC_SCALAR(p_lhs, +, p_lhs, p_rhs);
    return p_lhs;
}
#pragma endregion VECTOR_MATH_ADD

#pragma region VECTOR_MATH_SUB
template<typename T>
    requires VectorN<T>
constexpr T operator-(const T& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_VEC(result, -, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires VectorN<T> && Arithmetic<U>
constexpr T operator-(const T& p_lhs, const U& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_SCALAR(result, -, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires VectorN<T> && Arithmetic<U>
constexpr T operator-(const U& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_SCALAR_VEC(result, -, p_lhs, p_rhs);
    return result;
}

template<typename T>
    requires VectorN<T>
constexpr T& operator-=(T& p_lhs, const T& p_rhs) {
    VECTOR_OPERATOR_VEC_VEC(p_lhs, -, p_lhs, p_rhs);
    return p_lhs;
}

template<typename T, typename U>
    requires VectorN<T> && Arithmetic<U>
constexpr T& operator-=(T& p_lhs, const U& p_rhs) {
    VECTOR_OPERATOR_VEC_SCALAR(p_lhs, -, p_lhs, p_rhs);
    return p_lhs;
}
#pragma endregion VECTOR_MATH_SUB

#pragma region VECTOR_MATH_MUL
template<typename T>
    requires VectorN<T>
constexpr T operator*(const T& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_VEC(result, *, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires VectorN<T> && Arithmetic<U>
constexpr T operator*(const T& p_lhs, const U& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_SCALAR(result, *, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires VectorN<T> && Arithmetic<U>
constexpr T operator*(const U& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_SCALAR_VEC(result, *, p_lhs, p_rhs);
    return result;
}

template<typename T>
    requires VectorN<T>
constexpr T& operator*=(T& p_lhs, const T& p_rhs) {
    VECTOR_OPERATOR_VEC_VEC(p_lhs, *, p_lhs, p_rhs);
    return p_lhs;
}

template<typename T, typename U>
    requires VectorN<T> && Arithmetic<U>
constexpr T& operator*=(T& p_lhs, const U& p_rhs) {
    VECTOR_OPERATOR_VEC_SCALAR(p_lhs, *, p_lhs, p_rhs);
    return p_lhs;
}
#pragma endregion VECTOR_MATH_MUL

#pragma region VECTOR_MATH_DIV
template<typename T>
    requires VectorN<T>
constexpr T operator/(const T& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_VEC(result, /, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires VectorN<T> && Arithmetic<U>
constexpr T operator/(const T& p_lhs, const U& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_SCALAR(result, /, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires VectorN<T> && Arithmetic<U>
constexpr T operator/(const U& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_SCALAR_VEC(result, /, p_lhs, p_rhs);
    return result;
}

template<typename T>
    requires VectorN<T>
constexpr T& operator/=(T& p_lhs, const T& p_rhs) {
    VECTOR_OPERATOR_VEC_VEC(p_lhs, /, p_lhs, p_rhs);
    return p_lhs;
}

template<typename T, typename U>
    requires VectorN<T> && Arithmetic<U>
constexpr T& operator/=(T& p_lhs, const U& p_rhs) {
    VECTOR_OPERATOR_VEC_SCALAR(p_lhs, /, p_lhs, p_rhs);
    return p_lhs;
}
#pragma endregion VECTOR_MATH_DIV

}  // namespace my

namespace my::math {

template<typename T>
    requires Arithmetic<T>
constexpr T Min(const T& p_lhs, const T& p_rhs) {
    return p_lhs < p_rhs ? p_lhs : p_rhs;
}

template<typename T>
    requires Arithmetic<T>
constexpr T Max(const T& p_lhs, const T& p_rhs) {
    return p_lhs > p_rhs ? p_lhs : p_rhs;
}

template<typename T>
    requires VectorN<T>
constexpr T Min(const T& p_lhs, const T& p_rhs) {
    constexpr int dim = sizeof(p_lhs) / sizeof(p_lhs.x);
    T result;
    for (int i = 0; i < dim; ++i) {
        result[i] = Min(p_lhs[i], p_rhs[i]);
    }
    return result;
}

template<typename T>
    requires VectorN<T>
constexpr T Max(const T& p_lhs, const T& p_rhs) {
    constexpr int dim = sizeof(p_lhs) / sizeof(p_lhs.x);
    T result;
    for (int i = 0; i < dim; ++i) {
        result[i] = Max(p_lhs[i], p_rhs[i]);
    }
    return result;
}

template<typename T>
    requires VectorN<T> || Arithmetic<T>
constexpr T Clamp(const T& p_value, const T& p_min, const T& p_max) {
    return Max(p_min, Min(p_value, p_max));
}

template<typename T>
    requires VectorNf<T>
constexpr T Lerp(const T& p_x, const T& p_y, float p_s) {
    return p_x * (1.0f - p_s) + p_s * p_y;
}

}  // namespace my::math
