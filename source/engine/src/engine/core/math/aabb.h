#pragma once
#include "box.h"
#include "intersection.h"

namespace my {

class AABB : public Box3 {
public:
    using Box3::Box;

    Vector3f Corner(int p_index) const;

    void ApplyMatrix(const Matrix4x4f& p_mat);

    bool Intersects(const AABB& p_aabb) const { return TestIntersection::AabbAabb(*this, p_aabb); }
    bool Intersects(Ray& p_ray) const { return TestIntersection::RayAabb(*this, p_ray); }

    static AABB FromCenterSize(const Vector3f& p_center, const Vector3f& p_size);

    friend class TestIntersection;
};

}  // namespace my
