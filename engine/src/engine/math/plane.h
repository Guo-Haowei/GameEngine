#pragma once
#include "vector.h"

namespace my {

struct Plane {
    Vector3f normal;
    float dist;

    Plane() = default;
    Plane(const Vector3f& p_normal, float p_dist) : normal(p_normal), dist(p_dist) {}

    float Distance(const Vector3f& p_point) const;
};

}  // namespace my
