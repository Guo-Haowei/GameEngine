#pragma once
#include "geomath.h"

namespace my {

class Ray;
class AABB;

class TestIntersection {
public:
    static bool AabbAabb(const AABB& p_aabb1, const AABB& p_aabb2);
    static bool RayAabb(const AABB& p_aabb, Ray& p_ray);
    static bool RayTriangle(const Vector3f& p_a, const Vector3f& p_b, const Vector3f& p_c, Ray& p_ray);
};

}  // namespace my