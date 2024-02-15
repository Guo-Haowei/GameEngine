#pragma once
#include "geomath.h"

namespace my {

struct Plane {
    vec3 m_normal;
    float m_dist;

    Plane() = default;
    Plane(const vec3& normal, float dist) : m_normal(normal), m_dist(dist) {}

    float distance(const vec3& p) const { return glm::dot(p, m_normal) + m_dist; }
};

}  // namespace my
