/// File: pass_tracer.cs.hlsl
#include "cbuffer.hlsl.h"
#include "shader_defines.hlsl.h"
#include "shader_resource_defines.hlsl.h"
#include "unordered_access_defines.hlsl.h"

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
uint WangHash(inout uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

// random number between 0 and 1
float Random(inout uint state) {
    return float(WangHash(state)) / 4294967296.0;
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
    return true;
}

bool HitScene(inout Ray p_ray) {
    bool anyHit = false;

    for (int i = 0; i < 3 * 300; ++i) {
        if (HitTriangle(p_ray, i)) {
            anyHit = true;
            return true;
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

Vector3f RayColor(inout Ray ray, inout uint state) {
    Vector3f radiance = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f throughput = Vector3f(1.0f, 1.0f, 1.0f);

    Sphere sphere;
    sphere.radius = 4.0f;
    sphere.A = Vector3f(0.0f, 0.0f, -3.0f);
    // if (HitSphere(ray, sphere)) {
    //     return throughput;
    // }

    for (int i = 0; i < 1; ++i) {
        bool any_hit = HitScene(ray);
        if (any_hit) {
            return throughput;
        }
    }
    // return throughput;
    return radiance;
}

[numthreads(16, 16, 1)] void main(uint3 p_dispatch_thread_id : SV_DISPATCHTHREADID) {
    // random seed
    // [0, width], [0, height]
    const int2 uv = int2(p_dispatch_thread_id.xy);
    // iPixelCoords += tileOffset;
    float2 fPixelCoords = float2(float(uv.x), float(uv.y));
    uint2 dims;
    u_PathTracerOutputImage.GetDimensions(dims.x, dims.y);

    uint seed = uint(uint(uv.x) * uint(1973) + uint(uv.y) * uint(9277) + uint(c_frameIndex) * uint(26699)) | uint(1);

    // [-0.5, 0.5]
    Vector2f jitter = Vector2f(Random(seed), Random(seed)) - 0.5;

    Vector3f rayDir;
    {
        // screen position from [-1, 1]
        Vector2f uvJitter = (fPixelCoords + jitter) / dims;
        Vector2f screen = 2.0 * uvJitter - 1.0;

        // adjust for aspect ratio
        Vector2f resolution = Vector2f(float(dims.x), float(dims.y));
        float aspectRatio = resolution.x / resolution.y;
        screen.y /= aspectRatio;
        float halfFov = c_cameraFovDegree;
        float camDistance = tan(halfFov * PI / 180.0);
        rayDir = Vector3f(screen, camDistance);
        float3x3 mat = float3x3(c_cameraRight, c_cameraUp, c_cameraForward);
        rayDir = normalize(mul(rayDir, mat));
    }

    Ray ray;
    ray.origin = c_cameraPosition;
    ray.direction = rayDir;
    ray.t = RAY_T_MAX;
    ray.hitIndex = -1;
    ray.uv = Vector2f(0.0f, 0.0f);

    Vector3f radiance = RayColor(ray, seed);

    Vector4f final_color = Vector4f(radiance, 1.0);
#if 0
    if (c_sceneDirty == 0) {
        vec4 accumulated = imageLoad(u_PathTracerOutputImage, iPixelCoords);
        float weight = accumulated.a;
        vec3 new_color = radiance + weight * accumulated.rgb;
        float new_weight = accumulated.a + 1.0;
        new_color /= new_weight;
        final_color = vec4(new_color, new_weight);
    }
#endif

    u_PathTracerOutputImage[uv] = final_color;
}
