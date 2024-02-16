#pragma once
#include "geomath.h"

namespace my {

class Ray;
class AABB;

class TestIntersection {
public:
    static bool aabb_aabb(const AABB& aabb1, const AABB& aabb2);
    static bool ray_aabb(const AABB& aabb, Ray& ray);
    static bool ray_triangle(const vec3& a, const vec3& b, const vec3& c, Ray& ray);
};

}  // namespace my