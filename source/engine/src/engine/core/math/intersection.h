#pragma once
#include "geomath.h"

namespace my {

class Ray;
class AABB;

class TestIntersection {
public:
    static bool aabbAabb(const AABB& p_aabb1, const AABB& p_aabb2);
    static bool rayAabb(const AABB& p_aabb, Ray& p_ray);
    static bool rayTriangle(const vec3& p_a, const vec3& p_b, const vec3& p_c, Ray& p_ray);
};

}  // namespace my