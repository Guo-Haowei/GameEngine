#pragma once
#include "swizzle.h"
#include "vector_base.h"

namespace my::math {

template<Arithmetic T>
struct Vector<T, 3> : VectorBase<T, 3> {
    using Base = VectorBase<T, 3>;
    using Self = Vector<T, 3>;

    WARNING_PUSH()
    WARNING_DISABLE(4201, "-Wgnu-anonymous-struct")
    WARNING_DISABLE(4201, "-Wnested-anon-types")
    WARNING_DISABLE(4201, "-Wpadded")
    // clang-format off
    union {
        struct { T x, y, z; };
        struct { T r, g, b; };

        VECTOR3_SWIZZLE2;
        VECTOR3_SWIZZLE3;
    };
    // clang-format on
    WARNING_POP()

    explicit constexpr Vector() = default;

    explicit constexpr Vector(T p_v) : x(p_v),
                                       y(p_v),
                                       z(p_v) {
    }

    explicit constexpr Vector(T p_x, T p_y, T p_z) : x(p_x),
                                                     y(p_y),
                                                     z(p_z) {
    }

    template<Arithmetic U>
        requires(!std::is_same<T, U>::value)
    explicit constexpr Vector(U p_x, U p_y, U p_z) : x(static_cast<T>(p_x)),
                                                     y(static_cast<T>(p_y)),
                                                     z(static_cast<T>(p_z)) {
    }

    template<Arithmetic U>
        requires(!std::is_same<T, U>::value)
    explicit Vector(const Vector<U, 3>& p_rhs) : x(static_cast<T>(p_rhs.x)),
                                                 y(static_cast<T>(p_rhs.y)),
                                                 z(static_cast<T>(p_rhs.z)) {
    }

    explicit constexpr Vector(const Vector<T, 2>& p_vec, T p_z) : x(p_vec.x),
                                                                  y(p_vec.y),
                                                                  z(p_z) {
    }

    template<int N, int A, int B, int C>
    constexpr Vector(const Swizzle3<T, N, A, B, C, -1>& p_swizzle) : x(p_swizzle.d[A]),
                                                                     y(p_swizzle.d[B]),
                                                                     z(p_swizzle.d[C]) {
    }

    static const Self Zero;
    static const Self One;
    static const Self UnitX;
    static const Self UnitY;
    static const Self UnitZ;
};

template<Arithmetic T>
const Vector<T, 3> Vector<T, 3>::Zero(static_cast<T>(0));
template<Arithmetic T>
const Vector<T, 3> Vector<T, 3>::One(static_cast<T>(1));
template<Arithmetic T>
const Vector<T, 3> Vector<T, 3>::UnitX(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
template<Arithmetic T>
const Vector<T, 3> Vector<T, 3>::UnitY(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));
template<Arithmetic T>
const Vector<T, 3> Vector<T, 3>::UnitZ(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));

}  // namespace my::math
