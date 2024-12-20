#include "vector.test.h"

namespace my::math::detail {

// add
TEST(vector_math, add) {
    {
        Vector4u vec1(1, 2, 3, 4);
        Vector4u vec2(5, 4, 3, 1);
        Vector4u result = vec1 + vec2;
        CHECK_VEC4(result, 6, 6, 6, 5);
    }
    {
        Vector3u vec1(1, 3, 4);
        Vector3u vec2(4, 3, 1);
        Vector3u result = vec1 + vec2;
        CHECK_VEC3(result, 5, 6, 5);
    }
}

TEST(vector_math, add_scalar) {
    {
        Vector4u vec1(1, 2, 3, 4);
        Vector4u result = vec1 + 2;
        CHECK_VEC4(result, 3, 4, 5, 6);
    }
    {
        Vector3u vec1(1, 3, 4);
        Vector3u result = 1 + vec1;
        CHECK_VEC3(result, 2, 4, 5);
    }
}

TEST(vector_math, add_asign_scalar) {
    {
        Vector3u vec1(1, 3, 4);
        vec1 += 4;
        CHECK_VEC3(vec1, 5, 7, 8);
    }
    {
        Vector3u vec1(1, 3, 4);
        vec1 += -1;
        CHECK_VEC3(vec1, 0, 2, 3);
    }
}

TEST(vector_math, add_assign) {
    {
        Vector2u vec1(1, 2);
        Vector2u vec2(3, 4);
        Vector2u vec3(5, 6);
        Vector2u vec4(7, 8);
        vec1 += vec3;
        vec2 += vec4;
        CHECK_VEC2(vec1, 6, 8);
        CHECK_VEC2(vec2, 10, 12);
        CHECK_VEC2(vec3, 5, 6);
        CHECK_VEC2(vec4, 7, 8);
    }
    {
        Vector4u vec1(1, 2, 3, 4);
        Vector4u vec2(5, 4, 3, 1);
        vec1 += vec2;
        CHECK_VEC4(vec1, 6, 6, 6, 5);
    }
}

// sub
TEST(vector_math, sub) {
    {
        Vector4u vec1(5, 4, 3, 1);
        Vector4u vec2(1, 2, 3, 4);
        Vector4u result = vec1 - vec2;
        CHECK_VEC4(result, 4, 2, 0, -3);
    }
    {
        Vector3i vec1(1, 3, 4);
        Vector3i vec2(4, 3, 1);
        Vector3i result = vec1 - vec2;
        CHECK_VEC3(result, -3, 0, 3);
    }
}

TEST(vector_math, sub_scalar) {
    {
        Vector4u vec1(1, 2, 3, 4);
        Vector4u result = vec1 - (-2);
        CHECK_VEC4(result, 3, 4, 5, 6);
    }
    {
        Vector3f vec1(1.0f, 3.0f, 4.0f);
        Vector3f result = 1 - vec1;
        CHECK_VEC3(result, -0.0f, -2.0f, -3.0f);
    }
}

TEST(vector_math, sub_asign_scalar) {
    {
        Vector3i vec1(1, 3, 4);
        vec1 -= 4;
        CHECK_VEC3(vec1, -3, -1, 0);
    }
    {
        Vector3u vec1(1, 3, 4);
        vec1 -= 1;
        CHECK_VEC3(vec1, 0, 2, 3);
    }
}

TEST(vector_math, sub_assign) {
    {
        Vector2i vec1(1, 2);
        Vector2i vec2(3, 4);
        Vector2i vec3(5, 6);
        Vector2i vec4(7, 8);
        vec1 -= vec3;
        vec2 -= vec4;
        CHECK_VEC2(vec1, -4, -4);
        CHECK_VEC2(vec2, -4, -4);
        CHECK_VEC2(vec3, 5, 6);
        CHECK_VEC2(vec4, 7, 8);
    }
    {
        Vector4i vec1(1, 2, 3, 4);
        Vector4i vec2(5, 4, 3, 1);
        vec1 -= vec2;
        CHECK_VEC4(vec1, -4, -2, 0, 3);
    }
}

// mul
TEST(vector_math, mul) {
    {
        Vector4u vec1(5, 4, 3, 1);
        Vector4u vec2(1, 2, 3, 4);
        Vector4u result = vec1 * vec2;
        CHECK_VEC4(result, 5, 8, 9, 4);
    }
    {
        Vector3i vec1(1, 3, 4);
        Vector3i vec2(4, 3, 1);
        Vector3i result = vec1 * vec2;
        CHECK_VEC3(result, 4, 9, 4);
    }
}

TEST(vector_math, mul_scalar) {
    {
        Vector4u vec1(1, 2, 3, 4);
        Vector4u result = vec1 * 2;
        CHECK_VEC4(result, 2, 4, 6, 8);
    }
    {
        Vector3f vec1(1.0f, 3.0f, 4.0f);
        Vector3f result = 1 * vec1;
        CHECK_VEC3(result, 1.0f, 3.0f, 4.0f);
    }
}

TEST(vector_math, mul_asign_scalar) {
    {
        Vector3i vec1(1, 3, 4);
        vec1 *= 4;
        CHECK_VEC3(vec1, 4, 12, 16);
    }
    {
        Vector3u vec1(1, 3, 4);
        vec1 *= 9;
        CHECK_VEC3(vec1, 9, 27, 36);
    }
}

TEST(vector_math, mul_assign) {
    {
        Vector2i vec1(1, 2);
        Vector2i vec2(3, 4);
        Vector2i vec3(5, 6);
        Vector2i vec4(7, 8);
        vec1 *= vec3;
        vec2 *= vec4;
        CHECK_VEC2(vec1, 5, 12);
        CHECK_VEC2(vec2, 21, 32);
        CHECK_VEC2(vec3, 5, 6);
        CHECK_VEC2(vec4, 7, 8);
    }
    {
        Vector4i vec1(1, 2, 3, 4);
        Vector4i vec2(5, 4, 3, 1);
        vec1 *= vec2;
        CHECK_VEC4(vec1, 5, 8, 9, 4);
    }
}

// div
TEST(vector_math, div) {
    {
        Vector4u vec1(5, 4, 3, 1);
        Vector4u vec2(1, 2, 3, 4);
        Vector4u result = vec1 / vec2;
        CHECK_VEC4(result, 5, 2, 1, 0);
    }
    {
        Vector3u vec1(12, 12, 4);
        Vector3u vec2(4, 3, 2);
        Vector3u result = vec1 / vec2;
        CHECK_VEC3(result, 3, 4, 2);
    }
}

TEST(vector_math, div_scalar) {
    {
        Vector4u vec1(1, 2, 3, 4);
        Vector4u result = vec1 / 2;
        CHECK_VEC4(result, 0, 1, 1, 2);
    }
    {
        Vector3u vec1(1, 3, 4);
        Vector3u result = 10 / vec1;
        CHECK_VEC3(result, 10, 3, 2);
    }
}

TEST(vector_math, div_asign_scalar) {
    {
        Vector3u vec1(40, 8, 4);
        vec1 /= 4;
        CHECK_VEC3(vec1, 10, 2, 1);
    }
    {
        Vector3i vec1(1, 3, 4);
        vec1 /= -1;
        CHECK_VEC3(vec1, -1, -3, -4);
    }
}

TEST(vector_math, div_assign) {
    {
        Vector2f vec1(21, 20);
        Vector2f vec2(8, 4);
        Vector2f vec3(7, 5);
        Vector2f vec4(2, 4);
        vec1 /= vec3;
        vec2 /= vec4;
        CHECK_VEC2(vec1, 3, 4);
        CHECK_VEC2(vec2, 4, 1);
        CHECK_VEC2(vec3, 7, 5);
        CHECK_VEC2(vec4, 2, 4);
    }
    {
        Vector4u vec1(6, 8, 3, 81);
        Vector4u vec2(5, 4, 3, 9);
        vec1 /= vec2;
        CHECK_VEC4(vec1, 1, 2, 1, 9);
    }
}

TEST(vector_math, lerp) {
    {
        const Vector2f a(4.0f, 1.0f);
        const Vector2f b(2.0f, 3.0f);
        auto vec = Lerp(a, b, 0.0f);
        CHECK_VEC2(vec, 4, 1);
        vec = Lerp(a, b, 1.0f);
        CHECK_VEC2(vec, 2, 3);
        vec = Lerp(a, b, 0.1f);
        CHECK_VEC2(vec, 3.8f, 1.2f);
        CHECK_VEC2(a, 4, 1);
        CHECK_VEC2(b, 2, 3);
    }
    {
        const Vector4f a(4.0f, 20.0f, 12.0f, 4.0f);
        const Vector4f b(2.0f, 4.0f, 6.0f, 8.0f);
        auto vec = Lerp(a, b, .5f);
        CHECK_VEC4(vec, 3, 12, 9, 6);
    }
}

TEST(vector_math, dot) {
    {
        const Vector2i a(4, 1);
        const Vector2i b(2, 3);
        EXPECT_EQ(Dot(a, b), 11);
    }
    {
        const Vector4i a(1, 2, 3, 4);
        const Vector4i b(2, 4, 6, 8);
        EXPECT_EQ(Dot(a, b), 60);
    }
}

}  // namespace my::math::detail
