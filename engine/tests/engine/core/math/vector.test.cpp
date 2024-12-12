#include "vector.h"

namespace my::math::detail {

TEST(Vector2, constructor) {
    {
        const Vector2f& vec = Vector2f::Zero;

        EXPECT_EQ(vec.x, 0.0f);
        EXPECT_EQ(vec.y, 0.0f);
    }
    {
        const Vector2f& vec = Vector2f::One;

        EXPECT_EQ(vec.x, 1.0f);
        EXPECT_EQ(vec.y, 1.0f);
    }
    {
        Vector2f vec(1.0, 2.0f);

        EXPECT_EQ(vec.x, 1.0f);
        EXPECT_EQ(vec.y, 2.0f);
    }
}

TEST(Vector4, constructor) {
    {
        const Vector4f& vec = Vector4f::Zero;

        EXPECT_EQ(vec.x, 0.0f);
        EXPECT_EQ(vec.y, 0.0f);
        EXPECT_EQ(vec.z, 0.0f);
        EXPECT_EQ(vec.w, 0.0f);
    }
    {
        const Vector4f& vec = Vector4f::One;

        EXPECT_EQ(vec.x, 1.0f);
        EXPECT_EQ(vec.y, 1.0f);
        EXPECT_EQ(vec.z, 1.0f);
        EXPECT_EQ(vec.w, 1.0f);
    }
    {
        Vector4f vec(Vector3f(1.0f, 2.0f, 3.0f), 4.0f);

        EXPECT_EQ(vec.x, 1.0f);
        EXPECT_EQ(vec.y, 2.0f);
        EXPECT_EQ(vec.z, 3.0f);
        EXPECT_EQ(vec.w, 4.0f);
    }
}

}  // namespace my::math::detail
