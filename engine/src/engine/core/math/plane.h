#pragma once
#include "vector_math.h"

namespace my {

struct Plane {
    NewVector3f normal;
    float dist;

    Plane() = default;
    Plane(const NewVector3f& p_normal, float p_dist) : normal(p_normal), dist(p_dist) {}

    float Distance(const NewVector3f& p_point) const;
};

}  // namespace my
