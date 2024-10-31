#pragma once
#include "box.h"
#include "intersection.h"

namespace my {

class AABB : public Box3 {
public:
    using Box3::Box;

    vec3 Corner(int p_index) const;

    void ApplyMatrix(const mat4& p_mat);

    bool Intersects(const AABB& p_aabb) const { return TestIntersection::AabbAabb(*this, p_aabb); }
    bool Intersects(Ray& p_ray) const { return TestIntersection::RayAabb(*this, p_ray); }

    static AABB FromCenterSize(const vec3& p_center, const vec3& p_size);

    friend class TestIntersection;
};

}  // namespace my
