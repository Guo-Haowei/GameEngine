#pragma once
#include "common.h"
#include "vector2.h"
#include "vector3.h"
#include "vector4.h"

namespace my::math {

static_assert(alignof(__m128) == 16);

static inline __m128 VectorAddSSE(__m128 p_lhs, __m128 p_rhs) {
    return _mm_add_ps(p_lhs, p_rhs);
}

static inline __m128 VectorSubSSE(__m128 p_lhs, __m128 p_rhs) {
    return _mm_sub_ps(p_lhs, p_rhs);
}

static inline __m128 VectorMulSSE(__m128 p_lhs, __m128 p_rhs) {
    return _mm_mul_ps(p_lhs, p_rhs);
}

static inline __m128 VectorDivSSE(__m128 p_lhs, __m128 p_rhs) {
    return _mm_div_ps(p_lhs, p_rhs);
}

inline Vector<float, 4> operator+(const Vector<float, 4>& p_lhs, const Vector<float, 4>& p_rhs) {
    Vector<float, 4> result;
    result.simd = VectorAddSSE(p_lhs.simd, p_rhs.simd);
    return result;
}

inline Vector<float, 4> operator+=(Vector<float, 4>& p_lhs, const Vector<float, 4>& p_rhs) {
    p_lhs.simd = VectorAddSSE(p_lhs.simd, p_rhs.simd);
    return p_lhs;
}

inline Vector<float, 4> operator-(const Vector<float, 4>& p_lhs, const Vector<float, 4>& p_rhs) {
    Vector<float, 4> result;
    result.simd = VectorSubSSE(p_lhs.simd, p_rhs.simd);
    return result;
}

inline Vector<float, 4> operator-=(Vector<float, 4>& p_lhs, const Vector<float, 4>& p_rhs) {
    p_lhs.simd = VectorSubSSE(p_lhs.simd, p_rhs.simd);
    return p_lhs;
}

inline Vector<float, 4> operator*(const Vector<float, 4>& p_lhs, const Vector<float, 4>& p_rhs) {
    Vector<float, 4> result;
    result.simd = VectorMulSSE(p_lhs.simd, p_rhs.simd);
    return result;
}

inline Vector<float, 4> operator*=(Vector<float, 4>& p_lhs, const Vector<float, 4>& p_rhs) {
    p_lhs.simd = VectorMulSSE(p_lhs.simd, p_rhs.simd);
    return p_lhs;
}

inline Vector<float, 4> operator/(const Vector<float, 4>& p_lhs, const Vector<float, 4>& p_rhs) {
    Vector<float, 4> result;
    result.simd = VectorDivSSE(p_lhs.simd, p_rhs.simd);
    return result;
}

inline Vector<float, 4> operator/=(Vector<float, 4>& p_lhs, const Vector<float, 4>& p_rhs) {
    p_lhs.simd = VectorDivSSE(p_lhs.simd, p_rhs.simd);
    return p_lhs;
}

}  // namespace my::math
