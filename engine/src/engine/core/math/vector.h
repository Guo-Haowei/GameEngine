#pragma once
#include "swizzle.h"

namespace my {

template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

struct VectorBaseClass {};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#pragma clang diagnostic ignored "-Wpadded"

template<typename T, int N>
    requires Arithmetic<T> && (N >= 2 && N <= 4)
struct VectorBase : VectorBaseClass {
    using Self = VectorBase<T, N>;

    constexpr T* Data() { return reinterpret_cast<T*>(this); }
    constexpr const T* Data() const { return reinterpret_cast<const T*>(this); }

    constexpr void Set(T p_value) {
        T* data = Data();
        data[0] = p_value;
        data[1] = p_value;
        if constexpr (N >= 3) {
            data[2] = p_value;
        }
        if constexpr (N >= 4) {
            data[3] = p_value;
        }
    }

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

    constexpr Vector2() {
        Base::Set(static_cast<T>(0));
    }

    constexpr Vector2(T p_value) {
        Base::Set(p_value);
    }

    constexpr Vector2(T p_x, T p_y) : x(p_x),
                                      y(p_y) {
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

    constexpr Vector3() {
        Base::Set(static_cast<T>(0));
    }

    constexpr Vector3(T p_value) {
        Base::Set(p_value);
    }

    constexpr Vector3(T p_x, T p_y, T p_z) : x(p_x),
                                             y(p_y),
                                             z(p_z) {
    }

    constexpr Vector3(Vector2<T> p_vec, T p_z) : x(p_vec.x),
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

    constexpr Vector4() {
        Base::Set(static_cast<T>(0));
    }

    constexpr Vector4(T p_value) {
        Base::Set(p_value);
    }

    constexpr Vector4(T p_x, T p_y, T p_z, T p_w) : x(p_x),
                                                    y(p_y),
                                                    z(p_z),
                                                    w(p_w) {
    }

    constexpr Vector4(Vector3<T> p_vec, T p_w) : x(p_vec.x),
                                                 y(p_vec.y),
                                                 z(p_vec.z),
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

template<typename T>
    requires std::is_base_of_v<VectorBaseClass, T>
constexpr T operator+(const T& p_lhs, const T& p_rhs) {
    constexpr int dim = sizeof(p_lhs) / sizeof(p_lhs.x);
    T result;
    result.x = p_lhs.x + p_rhs.x;
    result.y = p_lhs.y + p_rhs.y;
    if constexpr (dim >= 3) {
        result.z = p_lhs.z + p_rhs.z;
    }
    if constexpr (dim >= 4) {
        result.w = p_lhs.w + p_rhs.w;
    }
    return result;
}

template<typename T>
    requires std::is_base_of_v<VectorBaseClass, T>
constexpr T& operator+=(T& p_lhs, const T& p_rhs) {
    p_lhs = p_lhs + p_rhs;
    return p_lhs;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T operator+(const U& p_scalar, const T& p_vec) {
    constexpr int dim = sizeof(p_vec) / sizeof(p_vec.x);
    T result;
    result.x = p_vec.x + p_scalar;
    result.y = p_vec.y + p_scalar;
    if constexpr (dim >= 3) {
        result.z = p_vec.z + p_scalar;
    }
    if constexpr (dim >= 4) {
        result.w = p_vec.w + p_scalar;
    }
    return result;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T operator+(const T& p_vec, const U& p_scalar) {
    return p_scalar + p_vec;
}

template<typename T, typename U>
    requires std::is_base_of_v<VectorBaseClass, T> && Arithmetic<U>
constexpr T& operator+=(T& p_vec, const U& p_scalar) {
    p_vec += T(p_scalar);
    return p_vec;
}

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
    requires std::is_base_of_v<VectorBaseClass, T>
constexpr T Min(const T& p_lhs, const T& p_rhs) {
    constexpr int dim = sizeof(p_lhs) / sizeof(p_lhs.x);
    T result;
    for (int i = 0; i < dim; ++i) {
        result[i] = Min(p_lhs[i], p_rhs[i]);
    }
    return result;
}

template<typename T>
    requires std::is_base_of_v<VectorBaseClass, T>
constexpr T Max(const T& p_lhs, const T& p_rhs) {
    constexpr int dim = sizeof(p_lhs) / sizeof(p_lhs.x);
    T result;
    for (int i = 0; i < dim; ++i) {
        result[i] = Max(p_lhs[i], p_rhs[i]);
    }
    return result;
}

template<typename T>
    requires std::is_base_of_v<VectorBaseClass, T> || Arithmetic<T>
constexpr T Clamp(const T& p_value, const T& p_min, const T& p_max) {
    return Max(p_min, Min(p_value, p_max));
}

}  // namespace my::math
