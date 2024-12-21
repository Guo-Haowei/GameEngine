#pragma once
#include "swizzle.h"

namespace my {

template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;
template<typename T>
concept FloatingPoint = std::is_floating_point_v<T>;

WARNING_PUSH()
WARNING_DISABLE(4201, "-Wgnu-anonymous-struct")
WARNING_DISABLE(4201, "-Wnested-anon-types")
WARNING_DISABLE(4201, "-Wpadded")

template<Arithmetic T, int N>
    requires(N >= 2 && N <= 4)
struct VectorBase {
    using Self = VectorBase<T, N>;

    constexpr T* Data() { return reinterpret_cast<T*>(this); }
    constexpr const T* Data() const { return reinterpret_cast<const T*>(this); }

    constexpr void Set(const T* p_data) {
        T* data = Data();
        data[0] = p_data[0];
        data[1] = p_data[1];
        if constexpr (N >= 3) {
            data[2] = p_data[2];
        }
        if constexpr (N >= 4) {
            data[3] = p_data[3];
        }
    }

    constexpr T& operator[](int p_index) {
        return Data()[p_index];
    }

    constexpr const T& operator[](int p_index) const {
        return Data()[p_index];
    }
};

template<Arithmetic T, int N>
struct Vector;

template<Arithmetic T>
struct Vector<T, 2>;

template<Arithmetic T>
struct Vector<T, 3>;

template<Arithmetic T>
struct Vector<T, 4>;

template<typename T, int S, int N, int A, int B, int C, int D>
struct Swizzle;

template<typename T, int N, int A, int B, int C, int D>
using Swizzle2 = Swizzle<T, 2, N, A, B, C, D>;
template<typename T, int N, int A, int B, int C, int D>
using Swizzle3 = Swizzle<T, 3, N, A, B, C, D>;
template<typename T, int N, int A, int B, int C, int D>
using Swizzle4 = Swizzle<T, 4, N, A, B, C, D>;

template<Arithmetic T, int N, int A, int B, int C, int D>
    requires(N >= 2) and (A < N) and (B < N) and (C == -1) and (D == -1)
struct Swizzle<T, 2, N, A, B, C, D> {
    T d[N];

    Vector<T, 2> operator=(const Vector<T, 2>& p_vec) {
        return Vector<T, 2>(d[A] = p_vec.x, d[B] = p_vec.y);
    }

    operator Vector<T, 2>() {
        return Vector<T, 2>(d[A], d[B]);
    }
};

template<Arithmetic T, int N, int A, int B, int C, int D>
    requires(N >= 3) and (A < N) and (B < N) and (C < N) and (D == -1)
struct Swizzle<T, 3, N, A, B, C, D> {
    T d[N];

    Vector<T, 3> operator=(const Vector<T, 3>& p_vec) {
        return Vector<T, 3>(d[A] = p_vec.x, d[B] = p_vec.y, d[C] = p_vec.z);
    }

    operator Vector<T, 3>() {
        return Vector<T, 3>(d[A], d[B], d[C]);
    }
};

template<Arithmetic T, int N, int A, int B, int C, int D>
    requires(N >= 4) and (A < N) and (B < N) and (C < N) and (D < N)
struct Swizzle<T, 4, N, A, B, C, D> {
    T d[N];

    Vector<T, 4> operator=(const Vector<T, 4>& p_vec) {
        return Vector<T, 4>(d[A] = p_vec.x, d[B] = p_vec.y, d[C] = p_vec.z, d[D] = p_vec.w);
    }

    operator Vector<T, 4>() {
        return Vector<T, 4>(d[A], d[B], d[C], d[D]);
    }
};

template<Arithmetic T>
struct Vector<T, 2> : VectorBase<T, 2> {
    using Base = VectorBase<T, 2>;
    using Self = Vector<T, 2>;

    // clang-format off
    union {
        struct { T x, y; };
        struct { T r, g; };

        VECTOR2_SWIZZLE2;
    };
    // clang-format on

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
struct Vector<T, 3> : VectorBase<T, 3> {
    using Base = VectorBase<T, 3>;
    using Self = Vector<T, 3>;

    // clang-format off
    union {
        struct { T x, y, z; };
        struct { T r, g, b; };

        VECTOR3_SWIZZLE2;
        VECTOR3_SWIZZLE3;
    };
    // clang-format on

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
struct Vector<T, 4> : VectorBase<T, 4> {
    using Base = VectorBase<T, 4>;
    using Self = Vector<T, 4>;

    // clang-format off
    union {
        struct { T x, y, z, w; };
        struct { T r, g, b, a; };

        VECTOR4_SWIZZLE2;
        VECTOR4_SWIZZLE3;
        VECTOR4_SWIZZLE4;
    };
    // clang-format on

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

WARNING_POP()

template<Arithmetic T>
const Vector<T, 2> Vector<T, 2>::Zero(static_cast<T>(0));
template<Arithmetic T>
const Vector<T, 2> Vector<T, 2>::One(static_cast<T>(1));
template<Arithmetic T>
const Vector<T, 2> Vector<T, 2>::UnitX(static_cast<T>(1), static_cast<T>(0));
template<Arithmetic T>
const Vector<T, 2> Vector<T, 2>::UnitY(static_cast<T>(0), static_cast<T>(1));

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

using Vector2i = Vector<int, 2>;
using Vector3i = Vector<int, 3>;
using Vector4i = Vector<int, 4>;
using Vector2u = Vector<uint32_t, 2>;
using Vector3u = Vector<uint32_t, 3>;
using Vector4u = Vector<uint32_t, 4>;
using Vector2f = Vector<float, 2>;
using Vector3f = Vector<float, 3>;
using Vector4f = Vector<float, 4>;

static_assert(sizeof(Vector2f) == 8);
static_assert(sizeof(Vector3f) == 12);
static_assert(sizeof(Vector4f) == 16);
static_assert(sizeof(Vector2i) == 8);
static_assert(sizeof(Vector3i) == 12);
static_assert(sizeof(Vector4i) == 16);
static_assert(sizeof(Vector2u) == 8);
static_assert(sizeof(Vector3u) == 12);
static_assert(sizeof(Vector4u) == 16);

}  // namespace my
