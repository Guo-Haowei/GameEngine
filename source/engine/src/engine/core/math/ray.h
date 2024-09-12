#pragma once
#include "intersection.h"

namespace my {

class Ray {
public:
    Ray(const vec3& p_start, const vec3& p_end) : m_start(p_start), m_end(p_end), m_dist(1.0f) {}

    Ray inverse(const mat4& p_inverse_matrix) const;

    vec3 direction() const { return glm::normalize(m_end - m_start); }

    bool intersects(const AABB& p_aabb) { return TestIntersection::rayAabb(p_aabb, *this); }
    bool intersects(const vec3& p_a, const vec3& p_b, const vec3& p_c) { return TestIntersection::rayTriangle(p_a, p_b, p_c, *this); }

    // Used for inverse ray intersection update result
    void copyDist(const Ray& p_other) { m_dist = p_other.m_dist; }

private:
    const vec3 m_start;
    const vec3 m_end;
    float m_dist;  // if dist is in range of [0, 1.0f), it hits something

    friend class TestIntersection;
};

}  // namespace my
