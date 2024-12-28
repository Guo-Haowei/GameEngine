#include "vector.test.h"

namespace my::detail {

TEST(vector, vector2_constructor) {
    CHECK_VEC2(Vector2u::Zero, 0, 0);
    CHECK_VEC2(Vector2u::UnitX, 1, 0);
    CHECK_VEC2(Vector2u::UnitY, 0, 1);
    CHECK_VEC2(Vector2u::One, 1, 1);
    {
        Vector2f vec(1.0, 2.0f);
        CHECK_VEC2(vec, 1, 2);
    }
}

TEST(vector, vector3_constructor) {
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

TEST(vector, vector4_constructor) {
    CHECK_VEC4(Vector4f::Zero, 0, 0, 0, 0);
    CHECK_VEC4(Vector4f::One, 1, 1, 1, 1);
    CHECK_VEC4(Vector4f::UnitW, 0, 0, 0, 1);
    CHECK_VEC4(Vector4f::UnitY, 0, 1, 0, 0);
    {
        Vector4f vec(Vector3f(1.0f, 2.0f, 3.0f), 4.0f);
        CHECK_VEC4(vec, 1, 2, 3, 4);
    }
    {
        Vector4i vec(Vector2i(1, 2), 3, 4);
        CHECK_VEC4(vec, 1, 2, 3, 4);
    }
    {
        Vector4i vec(Vector2i(1, 2), Vector2i(3, 4));
        CHECK_VEC4(vec, 1, 2, 3, 4);
    }
}

TEST(vector, constructor_cast) {
    {
        int a = 1;
        int b = 2;
        Vector2f vec(a, b);
        CHECK_VEC2(vec, 1, 2);
    }
    {
        float a = 1.4f;
        float b = 2.2f;
        float c = -3.3f;
        Vector3i vec(a, b, c);
        CHECK_VEC3(vec, 1, 2, -3);
    }
    {
        int a = 5;
        int b = 2;
        int c = 3;
        int d = 7;
        Vector4f vec(a, b, c, d);
        CHECK_VEC4(vec, 5, 2, 3, 7);
    }
}

TEST(vector, access_operator) {
    Vector4f vec = Vector4f::UnitY;
    vec[2] = 1;

    Vector2f a = vec.yz;
    CHECK_VEC2(a, 1, 1);
}

}  // namespace my::detail
