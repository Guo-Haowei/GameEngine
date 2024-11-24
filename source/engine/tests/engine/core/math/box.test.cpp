#include "core/math/box.h"

namespace my::math {

TEST(box, constructor) {
    Box3 box{ Vector3f(1), Vector3f(10) };
    EXPECT_EQ(box.GetMin(), Vector3f(1));
    EXPECT_EQ(box.GetMax(), Vector3f(10));
}

TEST(box, expand_point) {
    Box3 box;
    box.ExpandPoint(Vector3f(1));

    EXPECT_EQ(box.GetMin(), Vector3f(1));
    EXPECT_EQ(box.GetMax(), Vector3f(1));

    box.ExpandPoint(Vector3f(3));
    EXPECT_EQ(box.GetMin(), Vector3f(1));
    EXPECT_EQ(box.GetMax(), Vector3f(3));

    box.ExpandPoint(Vector3f(-10));
    EXPECT_EQ(box.GetMin(), Vector3f(-10));
    EXPECT_EQ(box.GetMax(), Vector3f(3));
}

TEST(box, union_box) {
    Box3 box1{ Vector3f(-10), Vector3f(5) };
    Box3 box2{ Vector3f(-5), Vector3f(10) };

    box1.UnionBox(box2);
    EXPECT_EQ(box1.GetMin(), Vector3f(-10));
    EXPECT_EQ(box1.GetMax(), Vector3f(10));
}

TEST(box, intersect_box) {
    Box3 box1{ Vector3f(-10), Vector3f(5) };
    Box3 box2{ Vector3f(-5), Vector3f(10) };

    box1.IntersectBox(box2);
    EXPECT_EQ(box1.GetMin(), Vector3f(-5));
    EXPECT_EQ(box1.GetMax(), Vector3f(5));
}

TEST(box, center_and_size) {
    Box3 box{ Vector3f(-10), Vector3f(5) };

    EXPECT_EQ(box.Center(), Vector3f(-2.5));
    EXPECT_EQ(box.Size(), Vector3f(15));
}

}  // namespace my::math
