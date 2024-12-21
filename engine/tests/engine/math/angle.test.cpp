#include "engine/math/angle.h"

namespace my::math {

// @TODO: add more tests
TEST(Degree, constructor) {
    Degree a(90.0f);
    EXPECT_FLOAT_EQ(a.GetDegree(), 90.0f);
    EXPECT_FLOAT_EQ(a.GetRadians(), math::HalfPi());
}

TEST(Degree, clamp) {
    Degree a(-190.0f);
    a.Clamp(-180.0f, 180.0f);
    EXPECT_FLOAT_EQ(a.GetDegree(), -180.0f);
}

}  // namespace my::math
