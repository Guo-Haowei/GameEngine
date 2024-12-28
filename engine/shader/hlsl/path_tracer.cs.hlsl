/// File: pass_tracer.cs.hlsl
#include "cbuffer.hlsl.h"
#include "shader_defines.hlsl.h"
#include "shader_resource_defines.hlsl.h"
#include "unordered_access_defines.hlsl.h"

// @NOTE: include this at last
#include "path_tracer.hlsl.h"

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
#if 0
    Vector2f jitter = Vector2f(0.0f, 0.0f);
#else
    Vector2f jitter = Vector2f(Random(seed), Random(seed)) - 0.5;
#endif

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
        float camDistance = tan(halfFov * MY_PI / 180.0);
        rayDir = Vector3f(screen, camDistance);
        float3x3 mat = float3x3(c_cameraRight, c_cameraUp, c_cameraForward);
        rayDir = normalize(mul(rayDir, mat));
    }

    Ray ray;
    ray.origin = c_cameraPosition;
    ray.direction = rayDir;
    ray.invDir = 1.0f / ray.direction;
    ray.t = RAY_T_MAX;

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
