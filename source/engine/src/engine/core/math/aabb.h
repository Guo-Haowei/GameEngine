#pragma once
#include "box.h"
#include "intersection.h"

namespace my {

class AABB : public Box3 {
public:
    using Box3::Box;

    vec3 corner(int index) const;

    void apply_matrix(const mat4& mat);

    bool intersects(const AABB& aabb) const { return TestIntersection::aabb_aabb(*this, aabb); }
    bool intersects(Ray& ray) const { return TestIntersection::ray_aabb(*this, ray); }

    static AABB from_center_size(const vec3& p_center, const vec3& p_size);

    friend class TestIntersection;
};

}  // namespace my
