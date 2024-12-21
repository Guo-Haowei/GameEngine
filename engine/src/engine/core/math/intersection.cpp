#include "intersection.h"

#include "aabb.h"
#include "ray.h"

namespace my {

bool TestIntersection::AabbAabb(const AABB& p_aabb1, const AABB& p_aabb2) {
    AABB tmp{ p_aabb1 };
    tmp.IntersectBox(p_aabb2);
    bool result = true;
    result = result && tmp.m_min.x < tmp.m_max.x;
    result = result && tmp.m_min.y < tmp.m_max.y;
    result = result && tmp.m_min.z < tmp.m_max.z;
    return result;
}

bool TestIntersection::RayAabb(const AABB& p_aabb, Ray& p_ray) {
    const Vector3f direction = p_ray.m_end - p_ray.m_start;

    Vector3f inv_d = 1.0f / direction;
    Vector3f t0s = (p_aabb.m_min - p_ray.m_start) * inv_d;
    Vector3f t1s = (p_aabb.m_max - p_ray.m_start) * inv_d;

    Vector3f tsmaller = math::Min(t0s, t1s);
    Vector3f tbigger = math::Max(t0s, t1s);

    const float tmin = math::Max(-FLT_MAX, math::Max(tsmaller.x, math::Max(tsmaller.y, tsmaller.z)));
    const float tmax = math::Min(FLT_MAX, math::Min(tbigger.x, math::Min(tbigger.y, tbigger.z)));

    // check bounding box
    if (tmin >= tmax || tmin <= 0.0f || tmin >= p_ray.m_dist) {
        return false;
    }

    p_ray.m_dist = tmin;
    return true;
}

bool TestIntersection::RayTriangle(const Vector3f& p_a, const Vector3f& p_b, const Vector3f& p_c, Ray& p_ray) {
    // P = A + u(B - A) + v(C - A) => O - A = -tD + u(B - A) + v(C - A)
    // -tD + uAB + vAC = AO
    const Vector3f direction = p_ray.m_end - p_ray.m_start;
    const Vector3f ab = p_b - p_a;
    const Vector3f ac = p_c - p_a;
    Vector3f P = math::Cross(direction, ac);
    const float det = math::Dot(ab, P);
    if (det < math::Epsilon()) {
        return false;
    }

    const float inv_det = 1.0f / det;
    const Vector3f AO = p_ray.m_start - p_a;

    const Vector3f q = math::Cross(AO, ab);
    const float u = math::Dot(AO, P) * inv_det;
    const float v = math::Dot(direction, q) * inv_det;

    if (u < 0.0 || v < 0.0 || u + v > 1.0) {
        return false;
    }

    const float t = math::Dot(ac, q) * inv_det;
    if (t < math::Epsilon() || t >= p_ray.m_dist) {
        return false;
    }

    p_ray.m_dist = t;
    return true;
}

}  // namespace my
