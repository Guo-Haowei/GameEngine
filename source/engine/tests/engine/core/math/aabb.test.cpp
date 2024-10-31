#include "core/math/aabb.h"

namespace my::math {

TEST(aabb, from_center_size) {
    AABB aabb = AABB::FromCenterSize(vec3(10, 8, 6), vec3(4));
    EXPECT_EQ(aabb.Center(), vec3(10, 8, 6));
    EXPECT_EQ(aabb.Size(), vec3(4));
    EXPECT_EQ(aabb.GetMin(), vec3(8, 6, 4));
    EXPECT_EQ(aabb.GetMax(), vec3(12, 10, 8));
}

}  // namespace my::math
