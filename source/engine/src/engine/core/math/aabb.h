#pragma once
#include "box.h"
#include "intersection.h"

namespace my {

class AABB : public Box3 {
public:
    using Box3::Box;

    vec3 corner(int p_index) const;

    void applyMatrix(const mat4& p_mat);

    bool intersects(const AABB& p_aabb) const { return TestIntersection::aabbAabb(*this, p_aabb); }
    bool intersects(Ray& p_ray) const { return TestIntersection::rayAabb(*this, p_ray); }

    static AABB fromCenterSize(const vec3& p_center, const vec3& p_size);

    friend class TestIntersection;
};

}  // namespace my
