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
    const NewVector3f direction = p_ray.m_end - p_ray.m_start;

    NewVector3f inv_d = 1.0f / direction;
    NewVector3f t0s = (p_aabb.m_min - p_ray.m_start) * inv_d;
    NewVector3f t1s = (p_aabb.m_max - p_ray.m_start) * inv_d;

    NewVector3f tsmaller = math::Min(t0s, t1s);
    NewVector3f tbigger = math::Max(t0s, t1s);

    const float tmin = math::Max(-FLT_MAX, math::Max(tsmaller.x, math::Max(tsmaller.y, tsmaller.z)));
    const float tmax = math::Min(FLT_MAX, math::Min(tbigger.x, math::Min(tbigger.y, tbigger.z)));

    // check bounding box
    if (tmin >= tmax || tmin <= 0.0f || tmin >= p_ray.m_dist) {
        return false;
    }

    p_ray.m_dist = tmin;
    return true;
}

bool TestIntersection::RayTriangle(const NewVector3f& p_a, const NewVector3f& p_b, const NewVector3f& p_c, Ray& p_ray) {
    // P = A + u(B - A) + v(C - A) => O - A = -tD + u(B - A) + v(C - A)
    // -tD + uAB + vAC = AO
    const NewVector3f direction = p_ray.m_end - p_ray.m_start;
    const NewVector3f ab = p_b - p_a;
    const NewVector3f ac = p_c - p_a;
    NewVector3f P;
    {
#define CC(a) glm::vec3(a.x, a.y, a.z)
        auto tmp = glm::cross(CC(direction), CC(ac));
        P.x = tmp.x;
        P.y = tmp.y;
        P.z = tmp.z;
    }
    const float det = math::Dot(ab, P);
    if (det < math::Epsilon()) {
        return false;
    }

    const float inv_det = 1.0f / det;
    const NewVector3f AO = p_ray.m_start - p_a;

    NewVector3f q;
    {
        auto tmp = glm::cross(CC(AO), CC(ab));
        q.x = tmp.x;
        q.y = tmp.y;
        q.z = tmp.z;
    }
    const float u = math::Dot(AO, P) * inv_det;
    const float v = math::Dot(direction, q) * inv_det;

    if (u < 0.0 || v < 0.0 || u + v > 1.0) {
        return false;
    }

    const float t = math::Dot(ac, q) * inv_det;
    if (t < glm::epsilon<float>() || t >= p_ray.m_dist) {
        return false;
    }

    p_ray.m_dist = t;
    return true;
}

}  // namespace my
