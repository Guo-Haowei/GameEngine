#pragma once
#include "swizzle.h"
#include "vector_base.h"

namespace my::math {

template<Arithmetic T>
struct alignas(sizeof(T) * 4) Vector<T, 4> : VectorBase<T, 4> {
    using Base = VectorBase<T, 4>;
    using Self = Vector<T, 4>;

    WARNING_PUSH()
    WARNING_DISABLE(4201, "-Wgnu-anonymous-struct")
    WARNING_DISABLE(4201, "-Wnested-anon-types")
    WARNING_DISABLE(4201, "-Wpadded")
    // clang-format off
    union {
        struct { T x, y, z, w; };
        struct { T r, g, b, a; };
        VECTOR4_SWIZZLE2;
        VECTOR4_SWIZZLE3;
        VECTOR4_SWIZZLE4;
#if USING(MATH_ENABLE_SIMD_SSE)
        __m128 simd;
#elif USING(MATH_ENABLE_SIMD_NEON)
        float16x4_t simd;
#endif
    };
    // clang-format on
    WARNING_POP()

    explicit constexpr Vector() = default;

    explicit constexpr Vector(T p_v) : x(p_v), y(p_v), z(p_v), w(p_v) {
    }

    explicit constexpr Vector(T p_x, T p_y, T p_z, T p_w) : x(p_x),
                                                            y(p_y),
                                                            z(p_z),
                                                            w(p_w) {
    }

    template<typename U>
        requires Arithmetic<U> && (!std::is_same<T, U>::value)
    explicit constexpr Vector(U p_x, U p_y, U p_z, U p_w) : x(static_cast<T>(p_x)),
                                                            y(static_cast<T>(p_y)),
                                                            z(static_cast<T>(p_z)),
                                                            w(static_cast<T>(p_w)) {
    }

    template<Arithmetic U>
        requires(!std::is_same<T, U>::value)
    explicit Vector(const Vector<U, 4>& p_rhs) : x(static_cast<T>(p_rhs.x)),
                                                 y(static_cast<T>(p_rhs.y)),
                                                 z(static_cast<T>(p_rhs.z)),
                                                 w(static_cast<T>(p_rhs.w)) {
    }

    explicit constexpr Vector(const Vector<T, 3>& p_vec, T p_w) : x(p_vec.x),
                                                                  y(p_vec.y),
                                                                  z(p_vec.z),
                                                                  w(p_w) {
    }

    explicit constexpr Vector(const Vector<T, 2>& p_vec1, const Vector<T, 2>& p_vec2) : x(p_vec1.x),
                                                                                        y(p_vec1.y),
                                                                                        z(p_vec2.x),
                                                                                        w(p_vec2.y) {
    }

    explicit constexpr Vector(const Vector<T, 2>& p_vec, T p_z, T p_w) : x(p_vec.x),
                                                                         y(p_vec.y),
                                                                         z(p_z),
                                                                         w(p_w) {
    }

    template<int N, int A, int B, int C, int D>
    constexpr Vector(const Swizzle4<T, N, A, B, C, D>& p_swizzle) : x(p_swizzle.d[A]),
                                                                    y(p_swizzle.d[B]),
                                                                    z(p_swizzle.d[C]),
                                                                    w(p_swizzle.d[D]) {
    }

    static const Self Zero;
    static const Self One;
    static const Self UnitX;
    static const Self UnitY;
    static const Self UnitZ;
    static const Self UnitW;
};

template<Arithmetic T>
const Vector<T, 4> Vector<T, 4>::Zero(static_cast<T>(0));
template<Arithmetic T>
const Vector<T, 4> Vector<T, 4>::One(static_cast<T>(1));
template<Arithmetic T>
const Vector<T, 4> Vector<T, 4>::UnitX(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
template<Arithmetic T>
const Vector<T, 4> Vector<T, 4>::UnitY(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
template<Arithmetic T>
const Vector<T, 4> Vector<T, 4>::UnitZ(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));
template<Arithmetic T>
const Vector<T, 4> Vector<T, 4>::UnitW(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));

}  // namespace my::math
