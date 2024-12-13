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

}  // namespace my::math::detail
