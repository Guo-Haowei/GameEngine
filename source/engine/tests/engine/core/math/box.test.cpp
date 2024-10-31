#include "core/math/box.h"

namespace my::math {

TEST(box, constructor) {
    Box3 box{ vec3(1), vec3(10) };
    EXPECT_EQ(box.GetMin(), vec3(1));
    EXPECT_EQ(box.GetMax(), vec3(10));
}

TEST(box, expand_point) {
    Box3 box;
    box.ExpandPoint(vec3(1));

    EXPECT_EQ(box.GetMin(), vec3(1));
    EXPECT_EQ(box.GetMax(), vec3(1));

    box.ExpandPoint(vec3(3));
    EXPECT_EQ(box.GetMin(), vec3(1));
    EXPECT_EQ(box.GetMax(), vec3(3));

    box.ExpandPoint(vec3(-10));
    EXPECT_EQ(box.GetMin(), vec3(-10));
    EXPECT_EQ(box.GetMax(), vec3(3));
}

TEST(box, union_box) {
    Box3 box1{ vec3(-10), vec3(5) };
    Box3 box2{ vec3(-5), vec3(10) };

    box1.UnionBox(box2);
    EXPECT_EQ(box1.GetMin(), vec3(-10));
    EXPECT_EQ(box1.GetMax(), vec3(10));
}

TEST(box, intersect_box) {
    Box3 box1{ vec3(-10), vec3(5) };
    Box3 box2{ vec3(-5), vec3(10) };

    box1.IntersectBox(box2);
    EXPECT_EQ(box1.GetMin(), vec3(-5));
    EXPECT_EQ(box1.GetMax(), vec3(5));
}

TEST(box, center_and_size) {
    Box3 box{ vec3(-10), vec3(5) };

    EXPECT_EQ(box.Center(), vec3(-2.5));
    EXPECT_EQ(box.Size(), vec3(15));
}

}  // namespace my::math
