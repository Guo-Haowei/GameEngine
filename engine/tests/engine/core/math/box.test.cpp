#include "engine/core/math/box.h"

namespace my::math {

TEST(box, constructor) {
    Box3 box(NewVector3f(1), NewVector3f(10));
    EXPECT_EQ(box.GetMin(), NewVector3f(1));
    EXPECT_EQ(box.GetMax(), NewVector3f(10));
}

TEST(box, expand_point) {
    Box3 box;
    box.ExpandPoint(NewVector3f(1));

    EXPECT_EQ(box.GetMin(), NewVector3f(1));
    EXPECT_EQ(box.GetMax(), NewVector3f(1));

    box.ExpandPoint(NewVector3f(3));
    EXPECT_EQ(box.GetMin(), NewVector3f(1));
    EXPECT_EQ(box.GetMax(), NewVector3f(3));

    box.ExpandPoint(NewVector3f(-10));
    EXPECT_EQ(box.GetMin(), NewVector3f(-10));
    EXPECT_EQ(box.GetMax(), NewVector3f(3));
}

TEST(box, union_box) {
    Box3 box1{ NewVector3f(-10), NewVector3f(5) };
    Box3 box2{ NewVector3f(-5), NewVector3f(10) };

    box1.UnionBox(box2);
    EXPECT_EQ(box1.GetMin(), NewVector3f(-10));
    EXPECT_EQ(box1.GetMax(), NewVector3f(10));
}

TEST(box, intersect_box) {
    Box3 box1{ NewVector3f(-10), NewVector3f(5) };
    Box3 box2{ NewVector3f(-5), NewVector3f(10) };

    box1.IntersectBox(box2);
    EXPECT_EQ(box1.GetMin(), NewVector3f(-5));
    EXPECT_EQ(box1.GetMax(), NewVector3f(5));
}

TEST(box, center_and_size) {
    Box3 box{ NewVector3f(-10), NewVector3f(5) };

    EXPECT_EQ(box.Center(), NewVector3f(-2.5));
    EXPECT_EQ(box.Size(), NewVector3f(15));
}

}  // namespace my::math
