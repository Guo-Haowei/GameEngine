#pragma once
#include "swizzle.h"

namespace my {

template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

struct VectorBaseClass {};

template<typename T>
concept VectorN = std::is_base_of_v<VectorBaseClass, T>;

template<typename T>
concept VectorNf =
    std::is_base_of_v<VectorBaseClass, T> &&
    std::is_floating_point_v<decltype(std::declval<T>().x)>;

WARNING_PUSH()
WARNING_DISABLE(4201, "-Wgnu-anonymous-struct")
WARNING_DISABLE(4201, "-Wnested-anon-types")
WARNING_DISABLE(4201, "-Wpadded")

template<typename T, int N>
    requires Arithmetic<T> && (N >= 2 && N <= 4)
struct VectorBase : VectorBaseClass {
    using Self = VectorBase<T, N>;

    constexpr T* Data() { return reinterpret_cast<T*>(this); }
    constexpr const T* Data() const { return reinterpret_cast<const T*>(this); }

    constexpr void Set(T* p_data) {
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

template<typename T>
    requires Arithmetic<T>
struct Vector2;
template<typename T>
    requires Arithmetic<T>
struct Vector3;
template<typename T>
    requires Arithmetic<T>
struct Vector4;

template<typename T, int N, int A, int B, int C, int D>
    requires Arithmetic<T> && (A <= N) && (B <= N)
struct Swizzle2 {
    T d[N];

    Vector2<T> operator=(const Vector2<T>& p_vec) {
        return Vector2<T>(d[A] = p_vec.x, d[B] = p_vec.y);
    }

    operator Vector2<T>() {
        return Vector2<T>(d[A], d[B]);
    }
};

template<typename T, int N, int A, int B, int C, int D>
    requires Arithmetic<T> && (A <= N) && (B <= N) && (C <= N)
struct Swizzle3 {
    T d[N];

    Vector3<T> operator=(const Vector3<T>& p_vec) {
        return Vector3<T>(d[A] = p_vec.x, d[B] = p_vec.y, d[C] = p_vec.z);
    }

    operator Vector3<T>() {
        return Vector3<T>(d[A], d[B], d[C]);
    }
};

template<typename T, int N, int A, int B, int C, int D>
    requires Arithmetic<T> && (A <= N) && (B <= N) && (C <= N) && (D <= N)
struct Swizzle4 {
    T d[N];

    Vector4<T> operator=(const Vector4<T>& p_vec) {
        return Vector4<T>(d[A] = p_vec.x, d[B] = p_vec.y, d[C] = p_vec.z, d[D] = p_vec.w);
    }

    operator Vector4<T>() {
        return Vector4<T>(d[A], d[B], d[C], d[D]);
    }
};

template<typename T>
    requires Arithmetic<T>
struct Vector2 : VectorBase<T, 2> {
    using Base = VectorBase<T, 2>;
    using Self = Vector2<T>;

    // clang-format off
    union {
        struct { T x, y; };
        struct { T r, g; };

        VECTOR2_SWIZZLE2;
    };
    // clang-format on

    explicit constexpr Vector2() : x(0), y(0) {
    }

    explicit constexpr Vector2(T p_v) : x(p_v), y(p_v) {
    }

    constexpr Vector2(T p_x, T p_y) : x(p_x),
                                      y(p_y) {
    }

    template<typename U>
        requires Arithmetic<U> && (!std::is_same<T, U>::value)
    explicit constexpr Vector2(U p_x, U p_y) : x(static_cast<T>(p_x)),
                                               y(static_cast<T>(p_y)) {
    }

    template<int N, int A, int B>
    constexpr Vector2(const Swizzle2<T, N, A, B, -1, -1>& p_swizzle) : x(p_swizzle.d[A]),
                                                                       y(p_swizzle.d[B]) {
    }

    static const Self Zero;
    static const Self One;
    static const Self UnitX;
    static const Self UnitY;
};

template<typename T>
    requires Arithmetic<T>
struct Vector3 : VectorBase<T, 3> {
    using Base = VectorBase<T, 3>;
    using Self = Vector3<T>;

    // clang-format off
    union {
        struct { T x, y, z; };
        struct { T r, g, b; };

        VECTOR3_SWIZZLE2;
        VECTOR3_SWIZZLE3;
    };
    // clang-format on

    explicit constexpr Vector3() : x(0),
                                   y(0),
                                   z(0) {
    }

    explicit constexpr Vector3(T p_v) : x(p_v),
                                        y(p_v),
                                        z(p_v) {
    }

    explicit constexpr Vector3(T p_x, T p_y, T p_z) : x(p_x),
                                                      y(p_y),
                                                      z(p_z) {
    }

    template<typename U>
        requires Arithmetic<U> && (!std::is_same<T, U>::value)
    explicit constexpr Vector3(U p_x, U p_y, U p_z) : x(static_cast<T>(p_x)),
                                                      y(static_cast<T>(p_y)),
                                                      z(static_cast<T>(p_z)) {
    }

    explicit constexpr Vector3(Vector2<T> p_vec, T p_z) : x(p_vec.x),
                                                          y(p_vec.y),
                                                          z(p_z) {
    }

    template<int N, int A, int B, int C>
    constexpr Vector3(const Swizzle3<T, N, A, B, C, -1>& p_swizzle) : x(p_swizzle.d[A]),
                                                                      y(p_swizzle.d[B]),
                                                                      z(p_swizzle.d[C]) {
    }

    static const Self Zero;
    static const Self One;
    static const Self UnitX;
    static const Self UnitY;
    static const Self UnitZ;
};

template<typename T>
    requires Arithmetic<T>
struct Vector4 : VectorBase<T, 4> {
    using Base = VectorBase<T, 4>;
    using Self = Vector4<T>;

    // clang-format off
    union {
        struct { T x, y, z, w; };
        struct { T r, g, b, a; };

        VECTOR4_SWIZZLE2;
        VECTOR4_SWIZZLE3;
        VECTOR4_SWIZZLE4;
    };
    // clang-format on

    explicit constexpr Vector4() : x(0), y(0), z(0), w(0) {
    }

    explicit constexpr Vector4(T p_v) : x(p_v), y(p_v), z(p_v), w(p_v) {
    }

    explicit constexpr Vector4(T p_x, T p_y, T p_z, T p_w) : x(p_x),
                                                             y(p_y),
                                                             z(p_z),
                                                             w(p_w) {
    }

    template<typename U>
        requires Arithmetic<U> && (!std::is_same<T, U>::value)
    explicit constexpr Vector4(U p_x, U p_y, U p_z, U p_w) : x(static_cast<T>(p_x)),
                                                             y(static_cast<T>(p_y)),
                                                             z(static_cast<T>(p_z)),
                                                             w(static_cast<T>(p_w)) {
    }

    explicit constexpr Vector4(Vector3<T> p_vec, T p_w) : x(p_vec.x),
                                                          y(p_vec.y),
                                                          z(p_vec.z),
                                                          w(p_w) {
    }

    explicit constexpr Vector4(Vector2<T> p_vec1, Vector2<T> p_vec2) : x(p_vec1.x),
                                                                       y(p_vec1.y),
                                                                       z(p_vec2.x),
                                                                       w(p_vec2.y) {
    }

    explicit constexpr Vector4(Vector2<T> p_vec, T p_z, T p_w) : x(p_vec.x),
                                                                 y(p_vec.y),
                                                                 z(p_z),
                                                                 w(p_w) {
    }

    template<int N, int A, int B, int C, int D>
    constexpr Vector4(const Swizzle4<T, N, A, B, C, D>& p_swizzle) : x(p_swizzle.d[A]),
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

template<typename T>
    requires Arithmetic<T>
const Vector2<T> Vector2<T>::Zero(static_cast<T>(0));
template<typename T>
    requires Arithmetic<T>
const Vector2<T> Vector2<T>::One(static_cast<T>(1));
template<typename T>
    requires Arithmetic<T>
const Vector2<T> Vector2<T>::UnitX(static_cast<T>(1), static_cast<T>(0));
template<typename T>
    requires Arithmetic<T>
const Vector2<T> Vector2<T>::UnitY(static_cast<T>(0), static_cast<T>(1));

template<typename T>
    requires Arithmetic<T>
const Vector3<T> Vector3<T>::Zero(static_cast<T>(0));
template<typename T>
    requires Arithmetic<T>
const Vector3<T> Vector3<T>::One(static_cast<T>(1));
template<typename T>
    requires Arithmetic<T>
const Vector3<T> Vector3<T>::UnitX(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
template<typename T>
    requires Arithmetic<T>
const Vector3<T> Vector3<T>::UnitY(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));
template<typename T>
    requires Arithmetic<T>
const Vector3<T> Vector3<T>::UnitZ(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));

template<typename T>
    requires Arithmetic<T>
const Vector4<T> Vector4<T>::Zero(static_cast<T>(0));
template<typename T>
    requires Arithmetic<T>
const Vector4<T> Vector4<T>::One(static_cast<T>(1));
template<typename T>
    requires Arithmetic<T>
const Vector4<T> Vector4<T>::UnitX(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
template<typename T>
    requires Arithmetic<T>
const Vector4<T> Vector4<T>::UnitY(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
template<typename T>
    requires Arithmetic<T>
const Vector4<T> Vector4<T>::UnitZ(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));
template<typename T>
    requires Arithmetic<T>
const Vector4<T> Vector4<T>::UnitW(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));

using NewVector2i = Vector2<int>;
using NewVector3i = Vector3<int>;
using NewVector4i = Vector4<int>;
using NewVector2u = Vector2<uint32_t>;
using NewVector3u = Vector3<uint32_t>;
using NewVector4u = Vector4<uint32_t>;
using NewVector2f = Vector2<float>;
using NewVector3f = Vector3<float>;
using NewVector4f = Vector4<float>;

static_assert(sizeof(NewVector2f) == 8);
static_assert(sizeof(NewVector3f) == 12);
static_assert(sizeof(NewVector4f) == 16);
static_assert(sizeof(NewVector2i) == 8);
static_assert(sizeof(NewVector3i) == 12);
static_assert(sizeof(NewVector4i) == 16);
static_assert(sizeof(NewVector2u) == 8);
static_assert(sizeof(NewVector3u) == 12);
static_assert(sizeof(NewVector4u) == 16);

}  // namespace my

