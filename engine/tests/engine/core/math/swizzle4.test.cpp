#include "vector.h"

namespace my::math::detail {

#define CHECK_VEC4(VEC, a,b,c,d) { EXPECT_EQ((VEC).x, a); EXPECT_EQ((VEC).y, b); EXPECT_EQ((VEC).z, c); EXPECT_EQ((VEC).w, d); }

TEST(Swizzle4, vector4_swizzle4_read) {
    {
        Vector4u vec = Vector4u(10, 14, 6, 2).wzyx;
        CHECK_VEC4(vec, 2, 6, 14, 10);
    }
    {
        Vector4u vec = Vector4u(10, 14, 6, 2).xxxx;
        CHECK_VEC4(vec, 10, 10, 10, 10);
    }
    {
        Vector4u vec = Vector4u(10, 14, 6, 2).yyww;
        CHECK_VEC4(vec, 14, 14, 2, 2);
    }
    {
        Vector4u vec = Vector4u(10, 14, 6, 2).xyzz;
        CHECK_VEC4(vec, 10, 14, 6, 6);
    }
}

TEST(Swizzle4, vector4_swizzle4_write) {
    {
        Vector4i vec;
        vec.xyzw = Vector4i(7, 6, 8, 9);
        CHECK_VEC4(vec, 7, 6, 8, 9);
    }
    {
        Vector4i vec;
        vec.wzyx = Vector4i(6, 7, 8, 9);
        CHECK_VEC4(vec, 9, 8, 7, 6);
    }
    {
        Vector4i vec;
        vec.wyzx = Vector4i(6, 7, 8, 9);
        CHECK_VEC4(vec, 9, 7, 8, 6);
    }
    {
        Vector4i vec;
        vec.xzyw = Vector4i(6, 7, 8, 9);
        CHECK_VEC4(vec, 6, 8, 7, 9);
    }
}

}  // namespace my::math::detail
