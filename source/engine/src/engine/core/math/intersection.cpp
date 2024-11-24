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

    Vector3f inv_d = Vector3f(1) / direction;
    Vector3f t0s = (p_aabb.m_min - p_ray.m_start) * inv_d;
    Vector3f t1s = (p_aabb.m_max - p_ray.m_start) * inv_d;

    Vector3f tsmaller = glm::min(t0s, t1s);
    Vector3f tbigger = glm::max(t0s, t1s);

    float tmin = glm::max(-FLT_MAX, glm::max(tsmaller.x, glm::max(tsmaller.y, tsmaller.z)));
    float tmax = glm::min(FLT_MAX, glm::min(tbigger.x, glm::min(tbigger.y, tbigger.z)));

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
    const Vector3f P = glm::cross(direction, ac);
    const float det = glm::dot(ab, P);
    if (det < glm::epsilon<float>()) {
        return false;
    }

    const float inv_det = 1.0f / det;
    const Vector3f AO = p_ray.m_start - p_a;

    const Vector3f q = glm::cross(AO, ab);
    const float u = glm::dot(AO, P) * inv_det;
    const float v = glm::dot(direction, q) * inv_det;

    if (u < 0.0 || v < 0.0 || u + v > 1.0) {
        return false;
    }

    const float t = dot(ac, q) * inv_det;
    if (t < glm::epsilon<float>() || t >= p_ray.m_dist) {
        return false;
    }

    p_ray.m_dist = t;
    return true;
}

}  // namespace my
