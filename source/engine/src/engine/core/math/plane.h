#pragma once
#include "geomath.h"

namespace my {

struct Plane {
    vec3 normal;
    float dist;

    Plane() = default;
    Plane(const vec3& p_normal, float p_dist) : normal(p_normal), dist(p_dist) {}

    float Distance(const vec3& p_point) const {
        return glm::dot(p_point, normal) + dist;
    }
};

}  // namespace my
