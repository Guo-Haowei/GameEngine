#include "engine/core/math/aabb.h"

// @TODO: remove this
#include "engine/core/math/geomath.h"

namespace my::math {

TEST(aabb, from_center_size) {
    AABB aabb = AABB::FromCenterSize(glm::vec3(10, 8, 6), glm::vec3(4));
    EXPECT_EQ(aabb.Center(), NewVector3f(10, 8, 6));
    EXPECT_EQ(aabb.Size(), NewVector3f(4));
    EXPECT_EQ(aabb.GetMin(), NewVector3f(8, 6, 4));
    EXPECT_EQ(aabb.GetMax(), NewVector3f(12, 10, 8));
}

}  // namespace my::math
