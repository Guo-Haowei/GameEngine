#pragma once
#include "swizzle.h"
#include "vector_base.h"

namespace my {

template<Arithmetic T>
struct Vector<T, 2> : VectorBase<T, 2> {
    using Base = VectorBase<T, 2>;
    using Self = Vector<T, 2>;

    WARNING_PUSH()
    WARNING_DISABLE(4201, "-Wgnu-anonymous-struct")
    WARNING_DISABLE(4201, "-Wnested-anon-types")
    WARNING_DISABLE(4201, "-Wpadded")
    // clang-format off
    union {
        struct { T x, y; };
        struct { T r, g; };

        VECTOR2_SWIZZLE2;
    };
    // clang-format on
    WARNING_POP()

    explicit constexpr Vector() = default;

    explicit constexpr Vector(T p_v) : x(p_v), y(p_v) {
    }

    constexpr Vector(T p_x, T p_y) : x(p_x),
                                     y(p_y) {
    }

    template<Arithmetic U>
        requires(!std::is_same<T, U>::value)
    explicit constexpr Vector(U p_x, U p_y) : x(static_cast<T>(p_x)),
                                              y(static_cast<T>(p_y)) {
    }

    template<Arithmetic U>
        requires(!std::is_same<T, U>::value)
    explicit Vector(const Vector<U, 2>& p_rhs) : x(static_cast<T>(p_rhs.x)),
                                                 y(static_cast<T>(p_rhs.y)) {
    }

    template<int N, int A, int B>
    constexpr Vector(const Swizzle2<T, N, A, B, -1, -1>& p_swizzle) : x(p_swizzle.d[A]),
                                                                      y(p_swizzle.d[B]) {
    }

    static const Self Zero;
    static const Self One;
    static const Self UnitX;
    static const Self UnitY;
};

template<Arithmetic T>
const Vector<T, 2> Vector<T, 2>::Zero(static_cast<T>(0));
template<Arithmetic T>
const Vector<T, 2> Vector<T, 2>::One(static_cast<T>(1));
template<Arithmetic T>
const Vector<T, 2> Vector<T, 2>::UnitX(static_cast<T>(1), static_cast<T>(0));
template<Arithmetic T>
const Vector<T, 2> Vector<T, 2>::UnitY(static_cast<T>(0), static_cast<T>(1));

}  // namespace my
