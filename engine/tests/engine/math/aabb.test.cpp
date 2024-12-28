#include "engine/math/aabb.h"

// @TODO: remove this
#include "engine/math/geomath.h"

namespace my {

TEST(aabb, from_center_size) {
    AABB aabb = AABB::FromCenterSize(Vector3f(10, 8, 6), Vector3f(4));
    EXPECT_EQ(aabb.Center(), Vector3f(10, 8, 6));
    EXPECT_EQ(aabb.Size(), Vector3f(4));
    EXPECT_EQ(aabb.GetMin(), Vector3f(8, 6, 4));
    EXPECT_EQ(aabb.GetMax(), Vector3f(12, 10, 8));
}

}  // namespace my
