#include "core/math/aabb.h"

namespace my::math {

TEST(AABB, from_center_size) {
    AABB aabb = AABB::fromCenterSize(vec3(10, 8, 6), vec3(4));
    EXPECT_EQ(aabb.center(), vec3(10, 8, 6));
    EXPECT_EQ(aabb.size(), vec3(4));
    EXPECT_EQ(aabb.getMin(), vec3(8, 6, 4));
    EXPECT_EQ(aabb.getMax(), vec3(12, 10, 8));
}

}  // namespace my::math
