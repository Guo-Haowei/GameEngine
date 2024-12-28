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

extern GpuPtVertex GlobalRtVertices[];
extern Vector3i GlobalRtIndices[];
extern GpuPtBvh GlobalRtBvhs[];
extern GpuPtMesh GlobalPtMeshes[];
#endif

#define EPSILON       1.1920929e-7
#define MAX_BOUNCE    10
#define RAY_T_MIN     1e-6
#define RAY_T_MAX     999999999.0
#define TRIANGLE_KIND 1
#define SPHERE_KIND   2

struct Ray {
    Vector3f origin;
    float t;
    Vector3f direction;
    Vector3f invDir;
};

struct HitResult {
    int hitTriangleId;  // @TODO: rename
    int hitMeshId;
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
Vector3f RandomUnitVector(inout uint p_state) {
    float z = Random(p_state) * 2.0 - 1.0;
    float a = Random(p_state) * MY_TWO_PI;
    float r = sqrt(1.0 - z * z);
    float x = r * cos(a);
    float y = r * sin(a);
    return Vector3f(x, y, z);
}

//------------------------------------------------------------------------------
// Common Ray Trace Functions
//------------------------------------------------------------------------------
// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
HitResult HitTriangle(inout Ray p_ray, int p_triangle_id) {
    // P = A + u(B - A) + v(C - A) => O - A = -tD + u(B - A) + v(C - A)
    // -tD + uAB + vAC = AO
    Vector3i indices = GlobalRtIndices[p_triangle_id];
    Vector3f A = GlobalRtVertices[indices.x].position;
    Vector3f B = GlobalRtVertices[indices.y].position;
    Vector3f C = GlobalRtVertices[indices.z].position;

    Vector3f AB = B - A;
    Vector3f AC = C - A;

    Vector3f P = cross(p_ray.direction, AC);
    float det = dot(AB, P);

    HitResult result;
    result.hitMeshId = -1;
    result.hitTriangleId = -1;
    result.uv = Vector2f(0.0f, 0.0f);
    if (det < EPSILON) {
        return result;
    }

    float invDet = 1.0 / det;
    Vector3f AO = p_ray.origin - A;

    Vector3f Q = cross(AO, AB);
    float u = dot(AO, P) * invDet;
    float v = dot(p_ray.direction, Q) * invDet;

    if (u < 0.0f || v < 0.0f || u + v > 1.0f) {
        return result;
    }

    float t = dot(AC, Q) * invDet;
    if (t >= p_ray.t || t < EPSILON) {
        return result;
    }

    p_ray.t = t;
    result.hitTriangleId = p_triangle_id;
    result.uv = Vector2f(u, v);
    return result;
}

// https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
bool HitBvh(in Ray p_ray, in Vector3f p_min, in Vector3f p_max) {
    Vector3f t_min = (p_min - p_ray.origin) * p_ray.invDir;
    Vector3f t_max = (p_max - p_ray.origin) * p_ray.invDir;

    Vector3f tsmaller = min(t_min, t_max);
    Vector3f tbigger = max(t_min, t_max);

    float dst_near = max(tsmaller.x, max(tsmaller.y, tsmaller.z));
    float dst_far = min(tbigger.x, min(tbigger.y, tbigger.z));

    bool hit = dst_far >= dst_near && dst_far > 0.0f;
    return hit;
    //(dst_near < dst_far) && (p_ray.t > dst_near) && (dst_far > 0.0f);
}

Ray TransformRay(in Ray p_ray, int p_mesh_id) {
    Matrix4x4f inversed = GlobalPtMeshes[p_mesh_id].transformInv;
    Ray ray;
    ray.origin = mul(inversed, Vector4f(p_ray.origin, 1.0f)).xyz;
    ray.direction = mul(inversed, Vector4f(p_ray.direction, 0.0f)).xyz;
    ray.invDir = 1.0f / ray.direction;  
    ray.t = p_ray.t;
    return ray;
}

// @TODO: hit result
HitResult HitScene(inout Ray p_ray) {
    HitResult res;
    res.hitMeshId = -1;
    res.hitTriangleId = -1;
    res.uv = Vector2f(0.0f, 0.0f);

    bool anyHit = false;

    // check if it hits all the objects
    for (int mesh_id = 0; mesh_id < 1; ++mesh_id) {
        Ray local_ray = TransformRay(p_ray, mesh_id);

        // @TODO: bvh start, it should stored in mesh
        int bvhIndex = 0;
        while (bvhIndex != -1) {
            GpuPtBvh bvh = GlobalRtBvhs[bvhIndex];
            if (HitBvh(local_ray, bvh.min, bvh.max)) {
                if (bvh.triangleIndex != -1) {
                    HitResult result = HitTriangle(local_ray, bvh.triangleIndex);
                    if (result.hitTriangleId != -1) {
                        res.hitTriangleId = result.hitTriangleId;
                        res.hitMeshId = mesh_id;
                        anyHit = true;
                    }
                }
                bvhIndex = bvh.hitIdx;
            } else {
                bvhIndex = bvh.missIdx;
            }
        }
        p_ray.t = local_ray.t;
    }

    return res;
}

Vector3f RayColor(inout Ray p_ray, inout uint state) {
    Vector3f radiance = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f throughput = Vector3f(1.0f, 1.0f, 1.0f);

    for (int i = 0; i < 1; ++i) {
        // check all objects
        HitResult result = HitScene(p_ray);
        if (result.hitMeshId != -1) {
            // @TODO: apply matrix
            Matrix4x4f transform = GlobalPtMeshes[result.hitMeshId].transform;

            Vector3i indices = GlobalRtIndices[result.hitTriangleId];
            Vector3f n1 = GlobalRtVertices[indices.x].normal;
            Vector3f n2 = GlobalRtVertices[indices.y].normal;
            Vector3f n3 = GlobalRtVertices[indices.z].normal;
            Vector3f n = n1 + result.uv.x * (n2 - n1) + result.uv.y * (n3 - n1);
            n = normalize(mul(transform, Vector4f(n, 0.0f)).xyz);
            return 0.5f * Vector3f(n) + 0.5f;
        }
    }

    return throughput;
    // return radiance;
}

#if 0
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
#endif

#if defined(__cplusplus)
}  // namespace my
#endif
