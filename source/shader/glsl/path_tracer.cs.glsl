/// File: path_tracer.cs.glsl
#include "../shader_resource_defines.hlsl.h"
#include "../cbuffer.hlsl.h"

layout(rgba32f, binding = 2) uniform image2D u_PathTracerOutputImage;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------
#ifndef PI
#define PI            3.14159265359
#define TWO_PI        6.28318530718
#endif
#define EPSILON       1e-6
#define MAX_BOUNCE    6
//#define MAX_BOUNCE    10
#define RAY_T_MIN     1e-6
#define RAY_T_MAX     9999999.0
#define TRIANGLE_KIND 1
#define SPHERE_KIND   2

struct Ray {
    vec3 origin;
    float t;
    vec3 direction;
    int material_id;
    vec3 hitNormal;
    vec2 hitUv;
    float hasAlbedoMap;
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
vec3 RandomUnitVector(inout uint state) {
    float z = Random(state) * 2.0 - 1.0;
    float a = Random(state) * TWO_PI;
    float r = sqrt(1.0 - z * z);
    float x = r * cos(a);
    float y = r * sin(a);
    return vec3(x, y, z);
}

//------------------------------------------------------------------------------
// Common Ray Trace Functions
//------------------------------------------------------------------------------
// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
bool HitTriangle(inout Ray ray, in gpu_geometry_t triangle) {
    // P = A + u(B - A) + v(C - A) => O - A = -tD + u(B - A) + v(C - A)
    // -tD + uAB + vAC = AO
    vec3 AB = triangle.B - triangle.A;
    vec3 AC = triangle.C - triangle.A;

    vec3 P = cross(ray.direction, AC);
    float det = dot(AB, P);

    if (det < EPSILON)
        return false;

    float invDet = 1.0 / det;
    vec3 AO = ray.origin - triangle.A;

    vec3 Q = cross(AO, AB);
    float u = dot(AO, P) * invDet;
    float v = dot(ray.direction, Q) * invDet;

    if (u < 0.0 || v < 0.0 || u + v > 1.0)
        return false;

    float t = dot(AC, Q) * invDet;
    if (t >= ray.t || t < EPSILON)
        return false;

    ray.t = t;
    ray.material_id = triangle.material_id;
    ray.hasAlbedoMap = triangle.hasAlbedoMap;
    vec3 norm = triangle.normal1 + u * (triangle.normal2 - triangle.normal1) + v * (triangle.normal3 - triangle.normal1);
    ray.hitNormal = norm;
    vec2 uv3 = vec2(triangle.uv3x, triangle.uv3y);
    ray.hitUv = triangle.uv1 + u * (triangle.uv2 - triangle.uv1) + v * (uv3 - triangle.uv1);

    return true;
}

bool HitSphere(inout Ray ray, in gpu_geometry_t sphere) {
    vec3 oc = ray.origin - sphere.A;
    float a = dot(ray.direction, ray.direction);
    float half_b = dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = half_b * half_b - a * c;

    float t = -half_b - sqrt(discriminant) / a;
    if (discriminant < EPSILON || t >= ray.t || t < EPSILON)
        return false;

    ray.t = t;
    vec3 p = ray.origin + t * ray.direction;
    ray.hitNormal = normalize(p - sphere.A);
    ray.material_id = sphere.material_id;

    return true;
}

// https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
bool HitBvh(in Ray ray, in gpu_bvh_t bvh) {
    vec3 invD = vec3(1.) / (ray.direction);
    vec3 t0s = (bvh.min - ray.origin) * invD;
    vec3 t1s = (bvh.max - ray.origin) * invD;

    vec3 tsmaller = min(t0s, t1s);
    vec3 tbigger = max(t0s, t1s);

    float tmin = max(RAY_T_MIN, max(tsmaller.x, max(tsmaller.y, tsmaller.z)));
    float tmax = min(RAY_T_MAX, min(tbigger.x, min(tbigger.y, tbigger.z)));

    return (tmin < tmax) && (ray.t > tmin);
}

bool HitScene(inout Ray ray) {
    bool anyHit = false;

    int bvhIdx = 0;
    while (bvhIdx != -1) {
        gpu_bvh_t bvh = GlobalBvhs[bvhIdx];
        if (HitBvh(ray, bvh)) {
            if (bvh.geomIdx != -1) {
                gpu_geometry_t geom = GlobalGeometries[bvh.geomIdx];
                if (geom.kind == TRIANGLE_KIND) {
                    if (HitTriangle(ray, geom)) {
                        anyHit = true;
                    }
                } else if (geom.kind == SPHERE_KIND) {
                    if (HitSphere(ray, geom)) {
                        anyHit = true;
                    }
                }
            }
            bvhIdx = bvh.hitIdx;
        } else {
            bvhIdx = bvh.missIdx;
        }
    }

    return anyHit;
}

vec2 SampleSphericalMap(in vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591, 0.3183);
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

vec3 RayColor(inout Ray ray, inout uint state) {
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);

    for (int i = 0; i < MAX_BOUNCE; ++i) {
        bool anyHit = HitScene(ray);

        if (anyHit) {
            ray.origin = ray.origin + ray.t * ray.direction;
            ray.t = RAY_T_MAX;
            gpu_material_t mat = GlobalMaterials[ray.material_id];
            float specularChance = Random(state) > mat.reflectChance ? 0.0 : 1.0;

            vec3 diffuseDir = normalize(ray.hitNormal + RandomUnitVector(state));
            vec3 reflectDir = reflect(ray.direction, ray.hitNormal);
            reflectDir = normalize(mix(reflectDir, diffuseDir, mat.roughness * mat.roughness));
            ray.direction = normalize(mix(diffuseDir, reflectDir, specularChance));

            vec3 diffuseColor = vec3(1.0);
            // vec3 diffuseColor = texture(albedoTexture, vec3(ray.hitUv, mat.albedoMapLevel)).rgb;
            // diffuseColor = mix(vec3(1.0), diffuseColor, ray.hasAlbedoMap);
            diffuseColor *= mat.albedo;

            radiance += mat.emissive * throughput;
            throughput *= diffuseColor;

        } else {
            //vec2 uv = SampleSphericalMap(normalize(ray.direction));
            //radiance += texture(envTexture, uv).rgb * throughput;
            radiance += vec3(0.2) * throughput;
            // radiance += vec3(0.5, 0.7, 1.0) * throughput;
            break;
        }
    }

    return radiance;
}

uint g_seed;

void main() {
    // random seed
    // [0, width], [0, height]
    ivec2 iPixelCoords = ivec2(gl_GlobalInvocationID.xy);
    // iPixelCoords += tileOffset;
    vec2 fPixelCoords = vec2(float(iPixelCoords.x), float(iPixelCoords.y));
    ivec2 dims = imageSize(u_PathTracerOutputImage);
    g_seed = uint(uint(iPixelCoords.x) * uint(1973) + uint(iPixelCoords.y) * uint(9277) + uint(c_frameIndex) * uint(26699)) | uint(1);

    // [-0.5, 0.5]
    vec2 jitter = vec2(Random(g_seed), Random(g_seed)) - 0.5;

    vec3 rayDir;
    {
        // screen position from [-1, 1]
        vec2 uvJitter = (fPixelCoords + jitter) / dims;
        vec2 screen = 2.0 * uvJitter - 1.0;

        // adjust for aspect ratio
        vec2 resolution = vec2(float(dims.x), float(dims.y));
        float aspectRatio = resolution.x / resolution.y;
        screen.y /= aspectRatio;
        float halfFov = c_cameraFov;
        float camDistance = tan(halfFov * PI / 180.0);
        rayDir = vec3(screen, camDistance);
        rayDir = normalize(mat3(c_cameraRight, c_cameraUp, c_cameraForward) * rayDir);
    }

    Ray ray;
    ray.origin = c_cameraPosition;
    ray.direction = rayDir;
    ray.t = RAY_T_MAX;

    vec3 radiance = RayColor(ray, g_seed);

    vec4 final_color = vec4(radiance, 1.0);
    if (c_sceneDirty == 0) {
        vec4 accumulated = imageLoad(u_PathTracerOutputImage, iPixelCoords);
        float weight = accumulated.a;
        vec3 new_color = radiance + weight * accumulated.rgb;
        float new_weight = accumulated.a + 1.0;
        new_color /= new_weight;
        final_color = vec4(new_color, new_weight);
    }

    imageStore(u_PathTracerOutputImage, iPixelCoords, final_color);
}
