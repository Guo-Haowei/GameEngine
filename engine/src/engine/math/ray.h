#pragma once
#include "intersection.h"

namespace my::math {

class Ray {
public:
    Ray(const Vector3f& p_start, const Vector3f& p_end) : m_start(p_start), m_end(p_end), m_dist(1.0f) {}

    Ray Inverse(const Matrix4x4f& p_inverse_matrix) const;

    Vector3f Direction() const;

    bool Intersects(const AABB& p_aabb) { return TestIntersection::RayAabb(p_aabb, *this); }

    bool Intersects(const Vector3f& p_a,
                    const Vector3f& p_b,
                    const Vector3f& p_c) {
        return TestIntersection::RayTriangle(p_a, p_b, p_c, *this);
    }

    // Used for inverse ray intersection update result
    void CopyDist(const Ray& p_other) { m_dist = p_other.m_dist; }

private:
    const Vector3f m_start;
    const Vector3f m_end;
    float m_dist;  // if dist is in range of [0, 1.0f), it hits something

    friend class TestIntersection;
};

}  // namespace my::math
