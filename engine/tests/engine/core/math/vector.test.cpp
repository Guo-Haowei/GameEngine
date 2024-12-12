#include "vector.test.h"

namespace my::math::detail {

TEST(Vector2, constructor) {
    CHECK_VEC2(Vector2u::Zero, 0, 0);
    CHECK_VEC2(Vector2u::UnitX, 1, 0);
    CHECK_VEC2(Vector2u::UnitY, 0, 1);
    CHECK_VEC2(Vector2u::One, 1, 1);
    {
        Vector2f vec(1.0, 2.0f);
        CHECK_VEC2(vec, 1, 2);
    }
}

TEST(Vector3, constructor) {
    CHECK_VEC3(Vector3u::Zero, 0, 0, 0);
    CHECK_VEC3(Vector3u::UnitX, 1, 0, 0);
    CHECK_VEC3(Vector3u::UnitZ, 0, 0, 1);
    CHECK_VEC3(Vector3u::One, 1, 1, 1);
    {
        Vector3f vec(1, 2, 3);
        CHECK_VEC3(vec, 1, 2, 3);
    }
    {
        Vector3f vec(Vector2f(1, 2), 3);
        CHECK_VEC3(vec, 1, 2, 3);
    }
}

TEST(Vector4, constructor) {
    CHECK_VEC4(Vector4f::Zero, 0, 0, 0, 0);
    CHECK_VEC4(Vector4f::One, 1, 1, 1, 1);
    CHECK_VEC4(Vector4f::UnitW, 0, 0, 0, 1);
    CHECK_VEC4(Vector4f::UnitY, 0, 1, 0, 0);
    {
        Vector4f vec(Vector3f(1.0f, 2.0f, 3.0f), 4.0f);
        CHECK_VEC4(vec, 1, 2, 3, 4);
    }
}

TEST(Vector4, access_operator) {
    Vector4f vec = Vector4f::UnitY;
    vec[2] = 1;

    Vector2 a = vec.yz;
    CHECK_VEC2(a, 1, 1);
}

TEST(Vector4, add) {
    {
        Vector4u vec1(1, 2, 3, 4);
        Vector4u vec2(5, 4, 3, 1);
        Vector4u sum = vec1 + vec2;
        CHECK_VEC4(sum, 6, 6, 6, 5);
    }
    {
        Vector3u vec1(1, 3, 4);
        Vector3u vec2(4, 3, 1);
        Vector3u sum = vec1 + vec2;
        CHECK_VEC3(sum, 5, 6, 5);
    }
}

TEST(Vector4, add_scalar) {
    {
        Vector4u vec1(1, 2, 3, 4);
        Vector4u sum = vec1 + 2;
        CHECK_VEC4(sum, 3, 4, 5, 6);
    }
    {
        Vector3u vec1(1, 3, 4);
        Vector3u sum = 1 + vec1;
        CHECK_VEC3(sum, 2, 4, 5);
    }
}

TEST(Vector4, add_asign_scalar) {
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

TEST(Vector, add_assign) {
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

}  // namespace my::math::detail
