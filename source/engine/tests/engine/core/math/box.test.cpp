#include "core/math/box.h"

namespace my::math {

TEST(box, constructor) {
    Box3 box{ vec3(1), vec3(10) };
    EXPECT_EQ(box.getMin(), vec3(1));
    EXPECT_EQ(box.getMax(), vec3(10));
}

TEST(box, expand_point) {
    Box3 box;
    box.expandPoint(vec3(1));

    EXPECT_EQ(box.getMin(), vec3(1));
    EXPECT_EQ(box.getMax(), vec3(1));

    box.expandPoint(vec3(3));
    EXPECT_EQ(box.getMin(), vec3(1));
    EXPECT_EQ(box.getMax(), vec3(3));

    box.expandPoint(vec3(-10));
    EXPECT_EQ(box.getMin(), vec3(-10));
    EXPECT_EQ(box.getMax(), vec3(3));
}

TEST(box, union_box) {
    Box3 box1{ vec3(-10), vec3(5) };
    Box3 box2{ vec3(-5), vec3(10) };

    box1.unionBox(box2);
    EXPECT_EQ(box1.getMin(), vec3(-10));
    EXPECT_EQ(box1.getMax(), vec3(10));
}

TEST(box, intersect_box) {
    Box3 box1{ vec3(-10), vec3(5) };
    Box3 box2{ vec3(-5), vec3(10) };

    box1.intersectBox(box2);
    EXPECT_EQ(box1.getMin(), vec3(-5));
    EXPECT_EQ(box1.getMax(), vec3(5));
}

TEST(box, center_and_size) {
    Box3 box{ vec3(-10), vec3(5) };

    EXPECT_EQ(box.center(), vec3(-2.5));
    EXPECT_EQ(box.size(), vec3(15));
}

}  // namespace my::math
