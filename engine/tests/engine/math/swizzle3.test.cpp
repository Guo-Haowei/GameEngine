#include "vector.test.h"

namespace my::detail {

TEST(Swizzle3, vector3_swizzle3_read) {
    {
        Vector3u vec = Vector3u(10, 14, 6).yxz;
        EXPECT_EQ(vec.x, 14);
        EXPECT_EQ(vec.y, 10);
        EXPECT_EQ(vec.z, 6);
    }

    const Vector3u vec3(1, 2, 3);
    // clang-format off
    #define TEST_HELPER(V, C) \
        { Vector3u vec = vec3.C##xx; EXPECT_EQ(vec.x, V); EXPECT_EQ(vec.y, 1); EXPECT_EQ(vec.z, 1); } \
        { Vector3u vec = vec3.C##xy; EXPECT_EQ(vec.x, V); EXPECT_EQ(vec.y, 1); EXPECT_EQ(vec.z, 2); } \
        { Vector3u vec = vec3.C##xz; EXPECT_EQ(vec.x, V); EXPECT_EQ(vec.y, 1); EXPECT_EQ(vec.z, 3); } \
        { Vector3u vec = vec3.C##yx; EXPECT_EQ(vec.x, V); EXPECT_EQ(vec.y, 2); EXPECT_EQ(vec.z, 1); } \
        { Vector3u vec = vec3.C##yy; EXPECT_EQ(vec.x, V); EXPECT_EQ(vec.y, 2); EXPECT_EQ(vec.z, 2); } \
        { Vector3u vec = vec3.C##yz; EXPECT_EQ(vec.x, V); EXPECT_EQ(vec.y, 2); EXPECT_EQ(vec.z, 3); } \
        { Vector3u vec = vec3.C##zx; EXPECT_EQ(vec.x, V); EXPECT_EQ(vec.y, 3); EXPECT_EQ(vec.z, 1); } \
        { Vector3u vec = vec3.C##zy; EXPECT_EQ(vec.x, V); EXPECT_EQ(vec.y, 3); EXPECT_EQ(vec.z, 2); } \
        { Vector3u vec = vec3.C##zz; EXPECT_EQ(vec.x, V); EXPECT_EQ(vec.y, 3); EXPECT_EQ(vec.z, 3); }
    // clang-format on

    TEST_HELPER(1, x);
    TEST_HELPER(2, y);
    TEST_HELPER(3, z);
#undef TEST_HELPER
}

TEST(Swizzle3, vector4_swizzle3_read) {
    const Vector4u vec4(7, 8, 9, 10);
    {
        Vector3u vec = vec4.yzx;
        EXPECT_EQ(vec.x, 8);
        EXPECT_EQ(vec.y, 9);
        EXPECT_EQ(vec.z, 7);
    }
    {
        Vector3u vec = vec4.xxx;
        EXPECT_EQ(vec.x, 7);
        EXPECT_EQ(vec.y, 7);
        EXPECT_EQ(vec.z, 7);
    }
    {
        Vector3u vec = vec4.xyw;
        EXPECT_EQ(vec.x, 7);
        EXPECT_EQ(vec.y, 8);
        EXPECT_EQ(vec.z, 10);
    }
    {
        Vector3u vec = vec4.ywz;
        EXPECT_EQ(vec.x, 8);
        EXPECT_EQ(vec.y, 10);
        EXPECT_EQ(vec.z, 9);
    }
    {
        Vector3u vec = vec4.wzy;
        EXPECT_EQ(vec.x, 10);
        EXPECT_EQ(vec.y, 9);
        EXPECT_EQ(vec.z, 8);
    }
}

TEST(Swizzle3, vector3_swizzle3_write) {
    {
        Vector3i vec;
        vec.xyz = Vector3i(7, 6, 8);
        EXPECT_EQ(vec.x, 7);
        EXPECT_EQ(vec.y, 6);
        EXPECT_EQ(vec.z, 8);
    }
    {
        Vector3i vec;
        vec.yzx = Vector3i(1, 2, 3);
        EXPECT_EQ(vec.x, 3);
        EXPECT_EQ(vec.y, 1);
        EXPECT_EQ(vec.z, 2);
    }
    {
        Vector3i vec;
        vec.zxy = Vector3i(1, 2, 3);
        EXPECT_EQ(vec.x, 2);
        EXPECT_EQ(vec.y, 3);
        EXPECT_EQ(vec.z, 1);
    }
    {
        Vector3i vec;
        vec.zyx = Vector3i(1, 2, 3);
        EXPECT_EQ(vec.x, 3);
        EXPECT_EQ(vec.y, 2);
        EXPECT_EQ(vec.z, 1);
    }
}

TEST(Swizzle3, vector4_swizzle3_write) {
    {
        Vector4i vec(0);
        vec.yzw = Vector3i(7, 6, 8);
        EXPECT_EQ(vec.x, 0);
        EXPECT_EQ(vec.y, 7);
        EXPECT_EQ(vec.z, 6);
        EXPECT_EQ(vec.w, 8);
    }
    {
        Vector4i vec(0);
        vec.wzx = Vector3i(1, 2, 3);
        EXPECT_EQ(vec.x, 3);
        EXPECT_EQ(vec.y, 0);
        EXPECT_EQ(vec.z, 2);
        EXPECT_EQ(vec.w, 1);
    }
    {
        Vector4i vec(8);
        vec.xyz = Vector3i(1, 2, 3);
        EXPECT_EQ(vec.x, 1);
        EXPECT_EQ(vec.y, 2);
        EXPECT_EQ(vec.z, 3);
        EXPECT_EQ(vec.w, 8);
    }
    {
        Vector4i vec(0);
        vec.yzw = Vector3i(1, 2, 3);
        EXPECT_EQ(vec.x, 0);
        EXPECT_EQ(vec.y, 1);
        EXPECT_EQ(vec.z, 2);
        EXPECT_EQ(vec.w, 3);
    }
}

}  // namespace my::detail
