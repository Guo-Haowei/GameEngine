/// File: path_tracer.hlsl.h
#if defined(__cplusplus)
#include <engine/math/vector.h>

#include <cmath>
namespace my {
#include "structured_buffer.hlsl.h"

#define inout
#define in
using uint = unsigned int;
using std::cos;
using std::sin;
using std::sqrt;

extern GpuTriangleVertex GlobalTriangleVertices[];
extern GpuTriangleIndex GlobalTriangleIndices[];
extern GpuBvhAccel GlobalBvhs[];
#endif

#ifndef PI
#define PI     3.14159265359
#define TWO_PI 6.28318530718
#endif
#define EPSILON       1e-6
#define MAX_BOUNCE    10
#define RAY_T_MIN     1e-6
#define RAY_T_MAX     9999999.0
#define TRIANGLE_KIND 1
#define SPHERE_KIND   2

struct Ray {
    Vector3f origin;
    float t;

    Vector3f direction;
    int hitIndex;

    Vector2f uv;
};

struct Sphere {
    Vector3f A;
    float radius;
};

//------------------------------------------------------------------------------
// Random function
//------------------------------------------------------------------------------
// https://blog.demofox.org/2020/05/25/casual-shadertoy-path-tracing-1-basic-camera-diffuse-emissive/
uint WangHash(inout uint p_seed) {
    p_seed = uint(p_seed ^ uint(61)) ^ uint(p_seed >> uint(16));
    p_seed *= uint(9);
    p_seed = p_seed ^ (p_seed >> 4);
    p_seed *= uint(0x27d4eb2d);
    p_seed = p_seed ^ (p_seed >> 15);
    return p_seed;
}

// random number between 0 and 1
float Random(inout uint p_state) {
    return float(WangHash(p_state)) / 4294967296.0;
}

// random unit vector
Vector3f RandomUnitVector(inout uint state) {
    float z = Random(state) * 2.0 - 1.0;
    float a = Random(state) * TWO_PI;
    float r = sqrt(1.0 - z * z);
    float x = r * cos(a);
    float y = r * sin(a);
    return Vector3f(x, y, z);
}

// how to transform a ray?
//Ray TransformRay(in Ray p_ray, Matrix4x4f p_transform) {
//
//}

//------------------------------------------------------------------------------
// Common Ray Trace Functions
//------------------------------------------------------------------------------
// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
bool HitTriangle(inout Ray ray, int p_triangle_id) {
    // P = A + u(B - A) + v(C - A) => O - A = -tD + u(B - A) + v(C - A)
    // -tD + uAB + vAC = AO
    Vector3i indices = GlobalTriangleIndices[p_triangle_id].indices;
    Vector3f A = GlobalTriangleVertices[indices.x].vertex;
    Vector3f B = GlobalTriangleVertices[indices.y].vertex;
    Vector3f C = GlobalTriangleVertices[indices.z].vertex;

    Vector3f AB = B - A;
    Vector3f AC = C - A;

    Vector3f P = cross(ray.direction, AC);
    float det = dot(AB, P);

    if (det < EPSILON) {
        return false;
    }

    float invDet = 1.0 / det;
    Vector3f AO = ray.origin - A;

    Vector3f Q = cross(AO, AB);
    float u = dot(AO, P) * invDet;
    float v = dot(ray.direction, Q) * invDet;

    if (u < 0.0 || v < 0.0 || u + v > 1.0) {
        return false;
    }

    float t = dot(AC, Q) * invDet;
    if (t >= ray.t || t < EPSILON) {
        return false;
    }

    ray.t = t;
    ray.uv.x = u;
    ray.uv.y = v;
    ray.hitIndex = p_triangle_id;
    return true;
}

// https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
bool HitBvh(in Ray ray, in GpuBvhAccel bvh) {
    Vector3f invD = 1.0f / (ray.direction);
    Vector3f t0s = (bvh.min - ray.origin) * invD;
    Vector3f t1s = (bvh.max - ray.origin) * invD;

    Vector3f tsmaller = min(t0s, t1s);
    Vector3f tbigger = max(t0s, t1s);

    float tmin = max(RAY_T_MIN, max(tsmaller.x, max(tsmaller.y, tsmaller.z)));
    float tmax = min(RAY_T_MAX, min(tbigger.x, min(tbigger.y, tbigger.z)));

    return (tmin < tmax) && (ray.t > tmin);
}

bool HitScene(inout Ray ray) {
    bool anyHit = false;

    int bvhIndex = 0;
    while (bvhIndex != -1) {
        GpuBvhAccel bvh = GlobalBvhs[bvhIndex];
        if (HitBvh(ray, bvh)) {
            if (bvh.triangleIndex != -1) {
                if (HitTriangle(ray, bvh.triangleIndex)) {
                    anyHit = true;
                }
            }
            bvhIndex = bvh.hitIdx;
        } else {
            bvhIndex = bvh.missIdx;
        }
    }

    return anyHit;
}

bool HitSphere(inout Ray ray, in Sphere sphere) {
    Vector3f oc = ray.origin - sphere.A;
    float a = dot(ray.direction, ray.direction);
    float half_b = dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = half_b * half_b - a * c;

    float t = -half_b - sqrt(discriminant) / a;
    if (discriminant < EPSILON || t >= ray.t || t < EPSILON) {
        return false;
    }

    ray.t = t;
    Vector3f p = ray.origin + t * ray.direction;
    // ray.hit_normal = normalize(p - sphere.A);
    // ray.material_id = sphere.material_id;
    return true;
}

#if defined(__cplusplus)
}  // namespace my
#endif
