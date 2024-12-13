#pragma once
#include "engine/core/math/vector.h"

#define CHECK_VEC2(VEC, a, b)  \
    {                          \
        EXPECT_EQ((VEC).x, a); \
        EXPECT_EQ((VEC).y, b); \
    }
#define CHECK_VEC3(VEC, a, b, c) \
    {                            \
        EXPECT_EQ((VEC).x, a);   \
        EXPECT_EQ((VEC).y, b);   \
        EXPECT_EQ((VEC).z, c);   \
    }
#define CHECK_VEC4(VEC, a, b, c, d) \
    {                               \
        EXPECT_EQ((VEC).x, a);      \
        EXPECT_EQ((VEC).y, b);      \
        EXPECT_EQ((VEC).z, c);      \
        EXPECT_EQ((VEC).w, d);      \
    }

namespace my::math::detail {

using Vector2i = Vector2<int>;
using Vector3i = Vector3<int>;
using Vector4i = Vector4<int>;
using Vector2u = Vector2<uint32_t>;
using Vector3u = Vector3<uint32_t>;
using Vector4u = Vector4<uint32_t>;
using Vector2f = Vector2<float>;
using Vector3f = Vector3<float>;
using Vector4f = Vector4<float>;

static_assert(sizeof(Vector2f) == 8);
static_assert(sizeof(Vector3f) == 12);
static_assert(sizeof(Vector4f) == 16);
static_assert(sizeof(Vector2i) == 8);
static_assert(sizeof(Vector3i) == 12);
static_assert(sizeof(Vector4i) == 16);
static_assert(sizeof(Vector2u) == 8);
static_assert(sizeof(Vector3u) == 12);
static_assert(sizeof(Vector4u) == 16);

}  // namespace my::math::detail

namespace my {

// @TODO: vector math
template<typename T>
    requires std::is_base_of_v<VectorBaseClass, T>
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
    requires std::is_base_of_v<VectorBaseClass, T>
constexpr T operator+(const T& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_VEC(result, +, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T operator+(const T& p_lhs, const U& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_SCALAR(result, +, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T operator+(const U& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_SCALAR_VEC(result, +, p_lhs, p_rhs);
    return result;
}

template<typename T>
    requires std::is_base_of_v<VectorBaseClass, T>
constexpr T& operator+=(T& p_lhs, const T& p_rhs) {
    VECTOR_OPERATOR_VEC_VEC(p_lhs, +, p_lhs, p_rhs);
    return p_lhs;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T& operator+=(T& p_lhs, const U& p_rhs) {
    VECTOR_OPERATOR_VEC_SCALAR(p_lhs, +, p_lhs, p_rhs);
    return p_lhs;
}
#pragma endregion VECTOR_MATH_ADD

#pragma region VECTOR_MATH_SUB
template<typename T>
    requires std::is_base_of_v<VectorBaseClass, T>
constexpr T operator-(const T& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_VEC(result, -, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T operator-(const T& p_lhs, const U& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_SCALAR(result, -, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T operator-(const U& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_SCALAR_VEC(result, -, p_lhs, p_rhs);
    return result;
}

template<typename T>
    requires std::is_base_of_v<VectorBaseClass, T>
constexpr T& operator-=(T& p_lhs, const T& p_rhs) {
    VECTOR_OPERATOR_VEC_VEC(p_lhs, -, p_lhs, p_rhs);
    return p_lhs;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T& operator-=(T& p_lhs, const U& p_rhs) {
    VECTOR_OPERATOR_VEC_SCALAR(p_lhs, -, p_lhs, p_rhs);
    return p_lhs;
}
#pragma endregion VECTOR_MATH_SUB

#pragma region VECTOR_MATH_MUL
template<typename T>
    requires std::is_base_of_v<VectorBaseClass, T>
constexpr T operator*(const T& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_VEC(result, *, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T operator*(const T& p_lhs, const U& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_SCALAR(result, *, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T operator*(const U& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_SCALAR_VEC(result, *, p_lhs, p_rhs);
    return result;
}

template<typename T>
    requires std::is_base_of_v<VectorBaseClass, T>
constexpr T& operator*=(T& p_lhs, const T& p_rhs) {
    VECTOR_OPERATOR_VEC_VEC(p_lhs, *, p_lhs, p_rhs);
    return p_lhs;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T& operator*=(T& p_lhs, const U& p_rhs) {
    VECTOR_OPERATOR_VEC_SCALAR(p_lhs, *, p_lhs, p_rhs);
    return p_lhs;
}
#pragma endregion VECTOR_MATH_MUL

#pragma region VECTOR_MATH_DIV
template<typename T>
    requires std::is_base_of_v<VectorBaseClass, T>
constexpr T operator/(const T& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_VEC(result, /, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T operator/(const T& p_lhs, const U& p_rhs) {
    T result;
    VECTOR_OPERATOR_VEC_SCALAR(result, /, p_lhs, p_rhs);
    return result;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T operator/(const U& p_lhs, const T& p_rhs) {
    T result;
    VECTOR_OPERATOR_SCALAR_VEC(result, /, p_lhs, p_rhs);
    return result;
}

template<typename T>
    requires std::is_base_of_v<VectorBaseClass, T>
constexpr T& operator/=(T& p_lhs, const T& p_rhs) {
    VECTOR_OPERATOR_VEC_VEC(p_lhs, /, p_lhs, p_rhs);
    return p_lhs;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T& operator/=(T& p_lhs, const U& p_rhs) {
    VECTOR_OPERATOR_VEC_SCALAR(p_lhs, /, p_lhs, p_rhs);
    return p_lhs;
}
#pragma endregion VECTOR_MATH_DIV

}  // namespace my