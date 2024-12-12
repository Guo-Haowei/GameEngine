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