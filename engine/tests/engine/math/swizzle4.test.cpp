#include "vector.test.h"

namespace my::detail {

TEST(Swizzle4, vector4_swizzle4_read) {
    {
        Vector4i vec = Vector4i(10, 14, 6, 2).wzyx;
        CHECK_VEC4(vec, 2, 6, 14, 10);
    }
    {
        Vector4i vec = Vector4i(10, 14, 6, 2).xxxx;
        CHECK_VEC4(vec, 10, 10, 10, 10);
    }
    {
        Vector4i vec = Vector4i(10, 14, 6, 2).yyww;
        CHECK_VEC4(vec, 14, 14, 2, 2);
    }
    {
        Vector4i vec = Vector4i(10, 14, 6, 2).xyzz;
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

}  // namespace my::detail
